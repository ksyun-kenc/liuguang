/*
 * Copyright 2020-present Ksyun
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "pch.h"

#include "video_encoder.h"

#include "app.hpp"
#include "engine.h"

extern App g_app;

bool VideoEncoder::Init(bool enable_nvenc,
                        uint64_t bitrate,
                        AVCodecID codec_id,
                        int gop,
                        std::string video_preset,
                        uint32_t quality) noexcept {
  enable_nvenc_ = enable_nvenc;
  bitrate_ = bitrate;
  codec_id_ = codec_id;
  gop_ = gop;
  video_preset_ = std::move(video_preset);
  quality_ = quality;

  HANDLE ev =
      CreateEvent(g_app.SA(), TRUE, FALSE, kVideoStartedEventName.data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return false;
  }
  started_event_.Attach(ev);

  ev = CreateEvent(g_app.SA(), TRUE, FALSE, kVideoStoppedEventName.data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return false;
  }
  stop_event_.Attach(ev);

  ev = CreateEvent(g_app.SA(), FALSE, FALSE,
                   kSharedVideoFrameReadyEventName.data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return false;
  }
  shared_frame_ready_event_.Attach(ev);

  HRESULT hr = shared_frame_info_.MapSharedMem(
      sizeof(SharedVideoFrameInfo), kSharedVideoFrameInfoFileMappingName.data(),
      nullptr, g_app.SA());
  if (FAILED(hr)) {
    std::cerr << "MapSharedMem() failed with 0x" << std::hex << hr << '\n';
    return false;
  }

  return true;
}

int VideoEncoder::Run() {
  ResetEvent(stop_event_);
  thread_ = std::thread(&VideoEncoder::EncodingThread, this);
  if (nullptr == thread_.native_handle()) {
    std::cerr << "Create thread failed with " << GetLastError() << '\n';
    return -1;
  }
  return 0;
}

void VideoEncoder::Stop() {
  Free(true);
}

int VideoEncoder::EncodingThread() {
  BOOST_SCOPE_EXIT_ALL(&) { Free(false); };

  const char* format_name = nullptr;
  if (AV_CODEC_ID_H264 == codec_id_) {
    format_name = "h264";
  } else if (AV_CODEC_ID_HEVC == codec_id_) {
    format_name = "hevc";
  }

  int error_code = avformat_alloc_output_context2(&format_context_, nullptr,
                                                  format_name, nullptr);
  if (error_code < 0) {
    return error_code;
  }

  SetEvent(started_event_);

  // wait for first video frame, to retrieve size.
  HANDLE events[] = {stop_event_, shared_frame_ready_event_};
  DWORD wait =
      WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
  if (WAIT_OBJECT_0 == wait) {
    ATLTRACE2(atlTraceUtil, 0, "%s: stopping.\n", __func__);
    return 0;
  } else if (WAIT_OBJECT_0 + 1 != wait) {
    int error_code = GetLastError();
    std::cout << "Unexpected WaitForMultipleObjects() return " << wait
              << ", error code " << error_code << ".\n";
    return error_code;
  }

  saved_frame_info_ = *static_cast<SharedVideoFrameInfo*>(shared_frame_info_);
  std::cout << "Video timestamp: " << saved_frame_info_.timestamp
            << ", type: " << static_cast<uint32_t>(saved_frame_info_.type)
            << ", dimension: " << saved_frame_info_.width << " * "
            << saved_frame_info_.height
            << ", format: " << saved_frame_info_.format << '\n';

  if (VideoFrameType::YUV == saved_frame_info_.type) {
    size_t yuv_size = 4 * saved_frame_info_.width * saved_frame_info_.height;
    size_t frames_size = sizeof(SharedVideoYuvFrames) +
                         sizeof(PackedVideoYuvFrame) * kNumberOfSharedFrames +
                         yuv_size * kNumberOfSharedFrames;

    HRESULT hr = shared_frames_.MapSharedMem(
        frames_size, kSharedVideoYuvFramesFileMappingName.data(), nullptr,
        g_app.SA());
    if (FAILED(hr)) {
      std::cerr << "MapSharedMem() failed with 0x" << std::hex << hr << '\n';
      return hr;
    }

    auto yuv_frames =
        reinterpret_cast<SharedVideoYuvFrames*>(shared_frames_.GetData());
    if (yuv_frames->data_size != sizeof(PackedVideoYuvFrame) + yuv_size) {
      std::cerr << "Invild data size " << yuv_frames->data_size
                << ", should be " << sizeof(PackedVideoYuvFrame) + yuv_size
                << '\n';
      return -1;
    }
  }

  AVCodec* codec = nullptr;
  error_code = AddStream(codec);
  if (error_code < 0) {
    return error_code;
  }

  AVDictionary* opts = nullptr;
  av_dict_set(&opts, "movflags",
              "frag_every_frame+dash+delay_moov+skip_sidx+skip_trailer", 0);
  error_code = Open(codec, &opts);
  av_dict_free(&opts);
  if (error_code < 0) {
    return error_code;
  }

  AVFrame* frame = nullptr;
  error_code = InitFrame(frame);
  if (error_code < 0) {
    std::cout << "Init frame failed with " << error_code << ".\n";
    return error_code;
  }
  BOOST_SCOPE_EXIT_ALL(&) { av_frame_free(&frame); };

  for (;;) {
    error_code = EncodeFrame(frame);

    wait = WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
    if (WAIT_OBJECT_0 == wait) {
      ATLTRACE2(atlTraceUtil, 0, "%s: stopping.\n", __func__);
      return 0;
    } else if (WAIT_OBJECT_0 + 1 != wait) {
      std::cout << "Unexpected WaitForMultipleObjects() return " << wait
                << ".\n";
      return -1;
    }
  }

  return 0;
}

void VideoEncoder::Free(bool wait_thread) {
  if (nullptr != started_event_) {
    ResetEvent(started_event_);
  }
  if (nullptr != stop_event_) {
    SetEvent(stop_event_);
  }
  if (wait_thread && thread_.joinable()) {
    thread_.join();
  }

  shared_frames_.Unmap();

  // TO-DO: Is it necessary to free stream_?

  avcodec_free_context(&codec_context_);
  if (nullptr != format_context_) {
    // int error = av_write_trailer(format_context_);
    // if (error < 0) {
    //  ATLTRACE2(atlTraceException, 0, "!av_write_trailer(), #%d, %s\n", error,
    //            GetAvErrorText(error));
    //}
    if (nullptr != format_context_->opaque) {
      av_free(format_context_->opaque);
      format_context_->opaque = nullptr;
    }
    avio_context_free(&format_context_->pb);
    avformat_free_context(format_context_);
    format_context_ = nullptr;
  }
}

int VideoEncoder::AddStream(AVCodec*& codec) {
  assert(nullptr == codec_context_);

  const char* codec_name = nullptr;
  if (AV_CODEC_ID_H264 == codec_id_) {
    if (enable_nvenc_) {
      codec_name = "h264_nvenc";
    } else {
      codec_name = "libx264";
    }
  } else if (AV_CODEC_ID_HEVC == codec_id_) {
    if (enable_nvenc_) {
      codec_name = "hevc_nvenc";
    } else {
      codec_name = "libx265";
    }
  } else {
    assert(false);
  }
  codec = avcodec_find_encoder_by_name(codec_name);
  if (nullptr == codec) {
    std::cerr << "Could not find encoder for " << codec_name << ".\n";
    return -1;
  }
  stream_ = avformat_new_stream(format_context_, nullptr);
  if (nullptr == stream_) {
    std::cerr << "Could not allocate stream for " << codec_name << ".\n";
    return -1;
  }
  stream_->id = format_context_->nb_streams - 1;
  stream_->codecpar->codec_tag = 0;

  return 0;
}

int VideoEncoder::Open(AVCodec* codec, AVDictionary** opts) {
  assert(nullptr == codec_context_);
  assert(nullptr != stream_);

  codec_context_ = avcodec_alloc_context3(codec);
  if (nullptr == codec_context_) {
    std::cerr << "Could not allocate codec context for " << codec->name
              << ".\n";
    return -1;
  }

  codec_context_->thread_count = 1;
  codec_context_->bit_rate = bitrate_;
  codec_context_->width = saved_frame_info_.width;
  codec_context_->height = saved_frame_info_.height;
  stream_->time_base = {1, kH264TimeBase};
  codec_context_->time_base = stream_->time_base;
  codec_context_->max_b_frames = 0;
  codec_context_->gop_size = gop_;
  codec_context_->pix_fmt = AV_PIX_FMT_YUV420P;
  codec_context_->codec_id = codec_id_;
  if (format_context_->oformat->flags & AVFMT_GLOBALHEADER) {
    codec_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  av_opt_set(codec_context_->priv_data, "preset", video_preset_.data(), 0);

  auto quality = std::to_string(quality_);
  if (enable_nvenc_) {
    av_opt_set(codec_context_->priv_data, "profile", "main", 0);
    av_opt_set(codec_context_->priv_data, "delay", "0", 0);
    if (AV_CODEC_ID_H264 == codec_id_) {
      av_opt_set(codec_context_->priv_data, "rc", "vbr", 0);
      av_opt_set(codec_context_->priv_data, "cq", quality.data(), 0);
    } else if (AV_CODEC_ID_HEVC == codec_id_) {
      av_opt_set(codec_context_->priv_data, "qp", quality.data(), 0);
    }
    av_opt_set(codec_context_->priv_data, "tune", "ull", 0);
    av_opt_set(codec_context_->priv_data, "zerolatency", "1", 0);
  } else {
    av_opt_set(codec_context_->priv_data, "crf", quality.data(), 0);
    av_opt_set(codec_context_->priv_data, "tune", "zerolatency", 0);

    if (AV_CODEC_ID_H264 == codec_id_) {
      av_opt_set(codec_context_->priv_data, "profile", "baseline", 0);
    } else if (AV_CODEC_ID_HEVC == codec_id_) {
      av_opt_set(codec_context_->priv_data, "profile", "main", 0);
    } else {
      assert(false);
    }
  }

  int error = avcodec_open2(codec_context_, codec, nullptr);
  if (error < 0) {
    ATLTRACE2(atlTraceException, 0, "!avcodec_open2(), #%d, %s\n", error,
              GetAvErrorText(error));
    return error;
  }

  error = avcodec_parameters_from_context(stream_->codecpar, codec_context_);
  if (error < 0) {
    ATLTRACE2(atlTraceException, 0,
              "!avcodec_parameters_from_context(), #%d, %s\n", error,
              GetAvErrorText(error));
    return error;
  }
  auto buffer = av_malloc(kInitialBufferSize);
  if (nullptr == buffer) {
    return AVERROR(ENOMEM);
  }
  format_context_->opaque = buffer;
  format_context_->pb =
      avio_alloc_context(reinterpret_cast<uint8_t*>(buffer), kInitialBufferSize,
                         AVIO_FLAG_WRITE, static_cast<EncoderInterface*>(this),
                         nullptr, Engine::OnWritePacket, nullptr);

  error = avformat_write_header(format_context_, opts);
  return error;
}

int VideoEncoder::InitFrame(AVFrame*& frame) const noexcept {
  assert(nullptr != codec_context_);
  assert(0 != codec_context_->width);
  assert(0 != codec_context_->height);

  frame = av_frame_alloc();
  if (nullptr == frame) {
    return AVERROR(ENOMEM);
  }

  frame->format = AV_PIX_FMT_YUV420P;
  frame->width = codec_context_->width;
  frame->height = codec_context_->height;
  frame->pict_type = AV_PICTURE_TYPE_NONE;

  int error_code = av_frame_get_buffer(frame, 0);
  if (error_code < 0) {
    av_frame_free(&frame);
    ATLTRACE2(atlTraceException, 0, "!av_frame_get_buffer(), #%d, %s\n",
              error_code, GetAvErrorText(error_code));
  }
  return error_code;
}

int VideoEncoder::EncodeFrame(AVFrame* frame) noexcept {
  assert(nullptr != codec_context_);
  assert(nullptr != frame);
  assert(nullptr != shared_frame_info_);
  assert(nullptr != shared_frames_);

  int error_code = 0;

  if (VideoFrameType::YUV == saved_frame_info_.type) {
    auto yuv_frames =
        reinterpret_cast<SharedVideoYuvFrames*>(shared_frames_.GetData());
    auto yuv_frame = reinterpret_cast<PackedVideoYuvFrame*>(yuv_frames->data);
    auto yuv_frame1 = reinterpret_cast<PackedVideoYuvFrame*>(
        yuv_frames->data + yuv_frames->data_size);

    if (yuv_frame->stats.timestamp < yuv_frame1->stats.timestamp) {
      yuv_frame = yuv_frame1;
    }
    ATLTRACE2(atlTraceUtil, 0, "latest video frame %llu\n",
              yuv_frame->stats.timestamp);

    error_code = EncodeYuvFrame(frame, yuv_frame->data);
  }

  return error_code;
}

int VideoEncoder::EncodeYuvFrame(AVFrame* frame, const uint8_t* yuv) noexcept {
  av_image_fill_arrays(frame->data, frame->linesize, yuv,
                       static_cast<AVPixelFormat>(frame->format), frame->width,
                       frame->height, 1);

  int64_t pts = 0;
  using namespace std::chrono;
  auto now = steady_clock::now();
  if (steady_clock::duration::zero() == startup_time_.time_since_epoch()) {
    startup_time_ = now;
  } else {
    using FpSeconds =
        std::chrono::duration<float, std::chrono::seconds::period>;
    static_assert(std::chrono::treat_as_floating_point<FpSeconds::rep>::value,
                  "Rep required to be floating point");

    pts = static_cast<int64_t>(
        duration_cast<FpSeconds>(now - startup_time_).count() /
        av_q2d(stream_->time_base));
  }
  frame->pts = pts;
  int error_code = avcodec_send_frame(codec_context_, frame);
  if (error_code < 0) {
    ATLTRACE2(atlTraceException, 0, "%s: !avcodec_send_frame(), #%d, %s.\n",
              __func__, error_code, GetAvErrorText(error_code));
    return error_code;
  }

  int written = -1;
  AVPacket pkt = {0};
  av_init_packet(&pkt);
  for (;;) {
    error_code = avcodec_receive_packet(codec_context_, &pkt);
    if (error_code < 0) {
      if (AVERROR(EAGAIN) == error_code && written == 0) {
        error_code = 0;
      }
      break;
    }
    BOOST_SCOPE_EXIT_ALL(&) { av_packet_unref(&pkt); };

    pkt.stream_index = stream_->index;
    written = av_write_frame(format_context_, &pkt);
    // flush the buffer.
    av_write_frame(format_context_, nullptr);
  }  // end of for

  return 0;
}
