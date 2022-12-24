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

#include "yuv/yuv.h"

using namespace regame;

bool VideoEncoder::Initialize(uint64_t bitrate,
                              AVCodecID codec_id,
                              HardwareEncoder hardware_encoder,
                              int gop,
                              std::string video_preset,
                              uint32_t quality) noexcept {
  hardware_encoder_ = hardware_encoder;
  bitrate_ = bitrate;
  SetCodecID(codec_id);
  gop_ = gop;
  video_preset_ = std::move(video_preset);
  quality_ = quality;

  HANDLE ev = CreateEvent(g_app.SA(), TRUE, FALSE,
                          object_namer_.Get(kVideoStartedEventName).data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return false;
  }
  started_event_.Attach(ev);

  ev = CreateEvent(g_app.SA(), TRUE, FALSE,
                   object_namer_.Get(kVideoStoppedEventName).data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return false;
  }
  stop_event_.Attach(ev);

  ev = CreateEvent(g_app.SA(), FALSE, FALSE,
                   object_namer_.Get(kSharedVideoFrameReadyEventName).data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return false;
  }
  shared_frame_ready_event_.Attach(ev);

  BOOL already_existed;
  HRESULT hr = shared_frame_info_.MapSharedMem(
      sizeof(SharedVideoFrameInfo),
      object_namer_.Get(kSharedVideoFrameInfoFileMappingName).data(),
      &already_existed, g_app.SA());
  if (FAILED(hr) && !already_existed) {
    APP_ERROR() << "MapSharedMem(info) failed with 0x" << std::hex << hr
                << '\n';
    return false;
  }

  return true;
}

int VideoEncoder::Run() {
  ResetEvent(stop_event_);
  thread_ = std::thread(&VideoEncoder::EncodingThread, this);
  if (nullptr == thread_.native_handle()) {
    APP_ERROR() << "Create thread failed with " << GetLastError() << '\n';
    return -1;
  }
  return 0;
}

void VideoEncoder::Stop() {
  Free(true);
}

int VideoEncoder::EncodingThread() {
  bool restart = false;
  BOOST_SCOPE_EXIT_ALL(&restart, this) {
    if (restart) {
      g_app.Engine().NotifyRestartVideoEncoder();
    } else {
      Free(restart);
    }
  };

  const char* format_name = nullptr;
  if (AV_CODEC_ID_H264 == GetCodecID()) {
    format_name = "h264";
  } else if (AV_CODEC_ID_HEVC == GetCodecID()) {
    format_name = "hevc";
  }

  int error_code = avformat_alloc_output_context2(&format_context_, nullptr,
                                                  format_name, nullptr);
  if (error_code < 0) {
    return error_code;
  }

  SetEvent(started_event_);

  // wait for first video frame, to retrieve size.
  HANDLE events[]{stop_event_, shared_frame_ready_event_};
  DWORD wait =
      WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
  if (WAIT_OBJECT_0 == wait) {
    ATLTRACE2(atlTraceUtil, 0, "%s: stopping.\n", __func__);
    return 0;
  } else if (WAIT_OBJECT_0 + 1 != wait) {
    int error_code = GetLastError();
    APP_WARNING() << "Unexpected WaitForMultipleObjects() return " << wait
                  << ", error code " << error_code << ".\n";
    return error_code;
  }

  SharedVideoFrameInfo* shared_frame_info = shared_frame_info_;
  saved_frame_info_ = *shared_frame_info;
  APP_INFO() << "Video timestamp: " << saved_frame_info_.timestamp
             << ", type: " << static_cast<uint32_t>(saved_frame_info_.type)
             << ", dimension: " << saved_frame_info_.width << " * "
             << saved_frame_info_.height
             << ", format: " << saved_frame_info_.format
             << ", window: " << saved_frame_info_.window << '\n';
  if (VideoFrameType::kNone == saved_frame_info_.type) {
    return ERROR_INVALID_DATA;
  }

  if (VideoFrameType::kTexture == saved_frame_info_.type) {
    HRESULT hr = shared_texture_frames_.OpenMapping(
        object_namer_.Get(kSharedVideoTextureFramesFileMappingName).data(),
        sizeof(SharedVideoTextureFrames));
    if (FAILED(hr)) {
      APP_ERROR() << "OpenMapping() failed with 0x" << std::hex << hr << '\n';
      restart = true;
      return hr;
    }
  } else {
    size_t pixel_size = saved_frame_info_.width * saved_frame_info_.height;
    size_t yuv_size;
    switch (saved_frame_info_.type) {
      case VideoFrameType::kI420:
        [[fallthrough]];
      case VideoFrameType::kJ420:
        yuv_size = pixel_size + (pixel_size >> 1);
        break;
      case VideoFrameType::kI422:
        [[fallthrough]];
      case VideoFrameType::kJ422:
        yuv_size = 2 * pixel_size;
        break;
      case VideoFrameType::kI444:
        yuv_size = 3 * pixel_size;
        break;
    }
    size_t frames_size = sizeof(SharedVideoYuvFrames) +
                         sizeof(PackedVideoYuvFrame) * kNumberOfSharedFrames +
                         yuv_size * kNumberOfSharedFrames;

    HRESULT hr = shared_yuv_frames_.OpenMapping(
        object_namer_.Get(kSharedVideoYuvFramesFileMappingName).data(),
        frames_size);
    if (FAILED(hr)) {
      APP_ERROR() << "OpenMapping() failed with 0x" << std::hex << hr << '\n';
      restart = true;
      return hr;
    }

    auto yuv_frames =
        reinterpret_cast<SharedVideoYuvFrames*>(shared_yuv_frames_.GetData());
    if (yuv_frames->data_size != sizeof(PackedVideoYuvFrame) + yuv_size) {
      APP_ERROR() << "Invild data size " << yuv_frames->data_size
                  << ", should be " << sizeof(PackedVideoYuvFrame) + yuv_size
                  << '\n';
      restart = true;
      return -1;
    }
  }

  const AVCodec* codec = nullptr;
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
  error_code = InitializeFrame(frame);
  if (error_code < 0) {
    APP_ERROR() << "Init frame failed with " << error_code << ".\n";
    return error_code;
  }
  BOOST_SCOPE_EXIT_ALL(&frame) {
    av_frame_free(&frame);
  };

  for (;;) {
    error_code = EncodeFrame(frame);

    wait = WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
    if (WAIT_OBJECT_0 == wait) {
      ATLTRACE2(atlTraceUtil, 0, "%s: stopping.\n", __func__);
      return 0;
    } else if (WAIT_OBJECT_0 + 1 != wait) {
      APP_WARNING() << "Unexpected WaitForMultipleObjects() return " << wait
                    << ".\n";
      return -1;
    }

    ATLTRACE2(atlTraceUtil, 0, "%s: %u * %u.\n", __func__,
              shared_frame_info->width, shared_frame_info->height);
    if (shared_frame_info->type != saved_frame_info_.type ||
        shared_frame_info->width != saved_frame_info_.width ||
        shared_frame_info->height != saved_frame_info_.height) {
      APP_INFO() << "Video dimension changed to " << shared_frame_info->width
                 << " * " << shared_frame_info->height << ".\n";
      restart = true;
      return 0;
    }
    if (saved_frame_info_.format != shared_frame_info->format) {
      saved_frame_info_.format = shared_frame_info->format;
    }
    if (saved_frame_info_.window != shared_frame_info->window) {
      saved_frame_info_.window = shared_frame_info->window;
    }
  }

  return 0;
}

void VideoEncoder::Free(bool wait_thread) {
  FreeHeader();

  if (nullptr != started_event_) {
    ResetEvent(started_event_);
  }
  if (nullptr != stop_event_) {
    SetEvent(stop_event_);
  }
  if (wait_thread && thread_.joinable()) {
    thread_.join();
  }

  staging_texture_.Release();
  for (auto& s : shared_textures_) {
    s.texture.Release();
  }
  shared_texture_frames_.Unmap();

  shared_yuv_frames_.Unmap();

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

int VideoEncoder::AddStream(const AVCodec*& codec) {
  assert(nullptr == codec_context_);

  const char* codec_name = nullptr;
  if (AV_CODEC_ID_H264 == GetCodecID()) {
    switch (hardware_encoder_) {
      case HardwareEncoder::AMF:
        codec_name = "h264_amf";
        break;
      case HardwareEncoder::NVENC:
        codec_name = "h264_nvenc";
        break;
      case HardwareEncoder::QSV:
        codec_name = "h264_qsv";
        break;
      default:
        codec_name = "libx264";
        break;
    }
  } else if (AV_CODEC_ID_HEVC == GetCodecID()) {
    switch (hardware_encoder_) {
      case HardwareEncoder::AMF:
        codec_name = "hevc_amf";
        break;
      case HardwareEncoder::NVENC:
        codec_name = "hevc_nvenc";
        break;
      case HardwareEncoder::QSV:
        codec_name = "hevc_qsv";
        break;
      default:
        codec_name = "libx265";
        break;
    }
  } else {
    assert(false);
  }
  codec = avcodec_find_encoder_by_name(codec_name);
  if (nullptr == codec) {
    APP_ERROR() << "Could not find encoder for " << codec_name << ".\n";
    return -1;
  }
  stream_ = avformat_new_stream(format_context_, nullptr);
  if (nullptr == stream_) {
    APP_ERROR() << "Could not allocate stream for " << codec_name << ".\n";
    return -1;
  }
  stream_->id = format_context_->nb_streams - 1;
  stream_->codecpar->codec_tag = 0;

  return 0;
}

int VideoEncoder::Open(const AVCodec* codec, AVDictionary** opts) {
  assert(nullptr == codec_context_);
  assert(nullptr != stream_);

  codec_context_ = avcodec_alloc_context3(codec);
  if (nullptr == codec_context_) {
    APP_ERROR() << "Could not allocate codec context for " << codec->name
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

  switch (saved_frame_info_.type) {
    case VideoFrameType::kI420:
      [[fallthrough]];
    case VideoFrameType::kTexture:
      if (HardwareEncoder::QSV == hardware_encoder_) {
        codec_context_->pix_fmt = AV_PIX_FMT_NV12;
      } else {
        codec_context_->pix_fmt = AV_PIX_FMT_YUV420P;
      }
      break;
    case VideoFrameType::kJ420:
      if (HardwareEncoder::QSV == hardware_encoder_) {
        codec_context_->pix_fmt = AV_PIX_FMT_NV12;
      } else {
        codec_context_->pix_fmt = AV_PIX_FMT_YUV420P;
        codec_context_->color_range = AVCOL_RANGE_JPEG;
      }
      break;
    case VideoFrameType::kJ422:
      codec_context_->color_range = AVCOL_RANGE_JPEG;
      [[fallthrough]];
    case VideoFrameType::kI422:
      codec_context_->pix_fmt = AV_PIX_FMT_YUV422P;
      break;
    case VideoFrameType::kI444:
      codec_context_->pix_fmt = AV_PIX_FMT_YUV444P;
      break;
  }
  codec_context_->codec_id = GetCodecID();
  codec_context_->flags |= AV_CODEC_FLAG_LOW_DELAY;
  if (format_context_->oformat->flags & AVFMT_GLOBALHEADER) {
    codec_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  auto quality = std::to_string(quality_);
  switch (hardware_encoder_) {
    case HardwareEncoder::AMF:
      // TO-DO
      av_opt_set(codec_context_->priv_data, "usage", "ultralowlatency", 0);
      av_opt_set(codec_context_->priv_data, "profile", "main", 0);
      av_opt_set(codec_context_->priv_data, "quality", video_preset_.data(), 0);
      av_opt_set(codec_context_->priv_data, "rc", "cqp", 0);
      av_opt_set(codec_context_->priv_data, "qp_i", quality.data(), 0);
      av_opt_set(codec_context_->priv_data, "qp_p", quality.data(), 0);
      av_opt_set(codec_context_->priv_data, "bf_ref", "0", 0);
      av_opt_set(codec_context_->priv_data, "enforce_hrd", "1", 0);
      av_opt_set(codec_context_->priv_data, "header_insertion_mode", "idr", 0);
      break;
    case HardwareEncoder::NVENC:
      av_opt_set(codec_context_->priv_data, "preset", video_preset_.data(), 0);
      av_opt_set(codec_context_->priv_data, "profile", "main", 0);
      av_opt_set(codec_context_->priv_data, "delay", "0", 0);
      av_opt_set(codec_context_->priv_data, "forced-idr", "1", 0);
      if (AV_CODEC_ID_H264 == GetCodecID()) {
        av_opt_set(codec_context_->priv_data, "rc", "vbr", 0);
        av_opt_set(codec_context_->priv_data, "cq", quality.data(), 0);
      } else if (AV_CODEC_ID_HEVC == GetCodecID()) {
        av_opt_set(codec_context_->priv_data, "qp", quality.data(), 0);
      }
      av_opt_set(codec_context_->priv_data, "tune", "ull", 0);
      av_opt_set(codec_context_->priv_data, "zerolatency", "1", 0);
      break;
    case HardwareEncoder::QSV:
      // TO-DO
      av_opt_set(codec_context_->priv_data, "preset", video_preset_.data(), 0);
      av_opt_set(codec_context_->priv_data, "profile", "main", 0);
      if (AV_CODEC_ID_HEVC == GetCodecID()) {
        av_opt_set(codec_context_->priv_data, "load_plugin", "hevc_hw", 0);
        av_opt_set(codec_context_->priv_data, "gpb", "0", 0);
      }
      break;
    default:
      av_opt_set(codec_context_->priv_data, "preset", video_preset_.data(), 0);
      av_opt_set(codec_context_->priv_data, "crf", quality.data(), 0);
      av_opt_set(codec_context_->priv_data, "forced-idr", "1", 0);
      av_opt_set(codec_context_->priv_data, "tune", "zerolatency", 0);

      if (AV_CODEC_ID_H264 == GetCodecID()) {
        av_opt_set(codec_context_->priv_data, "profile", "baseline", 0);
      } else if (AV_CODEC_ID_HEVC == GetCodecID()) {
        av_opt_set(codec_context_->priv_data, "profile", "main", 0);
      }
      break;
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
  format_context_->pb = avio_alloc_context(
      reinterpret_cast<uint8_t*>(buffer), kInitialBufferSize, 1,
      static_cast<Encoder*>(this), nullptr, Engine::OnWriteHeader, nullptr);
  error = avformat_write_header(format_context_, opts);
  if (error < 0) {
    ATLTRACE2(atlTraceException, 0, "!avformat_write_header(), #%d, %s\n",
              error, GetAvErrorText(error));
    return error;
  }
  format_context_->pb->write_packet = Engine::OnWritePacket;
  return error;
}

int VideoEncoder::InitializeFrame(AVFrame*& frame) const noexcept {
  assert(nullptr != codec_context_);
  assert(0 != codec_context_->width);
  assert(0 != codec_context_->height);

  frame = av_frame_alloc();
  if (nullptr == frame) {
    return AVERROR(ENOMEM);
  }

  switch (saved_frame_info_.type) {
    case VideoFrameType::kJ420:
      frame->color_range = AVCOL_RANGE_JPEG;
      [[fallthrough]];
    case VideoFrameType::kI420:
      [[fallthrough]];
    case VideoFrameType::kTexture:
      frame->format = AV_PIX_FMT_YUV420P;
      break;
    case VideoFrameType::kJ422:
      frame->color_range = AVCOL_RANGE_JPEG;
      [[fallthrough]];
    case VideoFrameType::kI422:
      frame->format = AV_PIX_FMT_YUV422P;
      break;
    case VideoFrameType::kI444:
      frame->format = AV_PIX_FMT_YUV444P;
      break;
  }
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
  assert(nullptr != shared_frame_info_);

  assert(VideoFrameType::kNone != saved_frame_info_.type);

  if (produce_keyframe_) {
    produce_keyframe_ = false;
    frame->pict_type = AV_PICTURE_TYPE_I;
  } else {
    frame->pict_type = AV_PICTURE_TYPE_NONE;
  }

  int error_code = 0;
  if (VideoFrameType::kTexture == saved_frame_info_.type) {
    assert(nullptr != shared_texture_frames_);
    HRESULT hr = GetSharedTexture();
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "!GetSharedTexture(), #0x%08X\n", hr);
      return hr;
    }

    CComPtr<IDXGISurface> staging_surface;
    hr = staging_texture_->QueryInterface(IID_PPV_ARGS(&staging_surface));
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                "!QueryInterface(IDXGISurface), #0x%08X\n", hr);
      return hr;
    }
    DXGI_MAPPED_RECT mapped_rect{};
    hr = staging_surface->Map(&mapped_rect, DXGI_MAP_READ);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "!Map(IDXGISurface), #0x%08X\n", hr);
      return hr;
    }
    BOOST_SCOPE_EXIT_ALL(&) {
      staging_surface->Unmap();
    };

    uint32_t width = saved_frame_info_.width;
    uint32_t height = saved_frame_info_.height;
    size_t pixel_size = width * height;

    const int uv_stride = width >> 1;
    uint8_t* y = yuv_frame_data_.data();
    uint8_t* u = y + pixel_size;
    uint8_t* v = u + (pixel_size >> 2);

    {
      // TO-DO: Texture is alwayse convert to YUV420P now!
      if (DXGI_FORMAT_R8G8B8A8_UNORM == saved_frame_info_.format) {
        ABGRToI420(mapped_rect.pBits, mapped_rect.Pitch, y, width, u, uv_stride,
                   v, uv_stride, width, height);
      } else if (DXGI_FORMAT_B8G8R8A8_UNORM == saved_frame_info_.format) {
        ARGBToI420(mapped_rect.pBits, mapped_rect.Pitch, y, width, u, uv_stride,
                   v, uv_stride, width, height);
      } else {
        ATLTRACE2(atlTraceException, 0, "Unsupported format %u.",
                  saved_frame_info_.format);
        return error_code;
      }
    }
    error_code = EncodeYuvFrame(frame, yuv_frame_data_.data());
  } else {
    assert(nullptr != shared_yuv_frames_);
    auto yuv_frames =
        reinterpret_cast<SharedVideoYuvFrames*>(shared_yuv_frames_.GetData());
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

  AVPacket* packet = av_packet_alloc();
  if (nullptr == packet) {
    return ERROR_OUTOFMEMORY;
  }
  BOOST_SCOPE_EXIT_ALL(&packet) {
    av_packet_free(&packet);
  };

  for (int written = -1;;) {
    error_code = avcodec_receive_packet(codec_context_, packet);
    if (error_code < 0) {
      if (AVERROR(EAGAIN) == error_code && written == 0) {
        error_code = 0;
      }
      break;
    }
    BOOST_SCOPE_EXIT_ALL(&packet) {
      av_packet_unref(packet);
    };

    packet->stream_index = stream_->index;
    written = av_write_frame(format_context_, packet);
    // flush the buffer.
    av_write_frame(format_context_, nullptr);
  }  // end of for

  return 0;
}

HRESULT VideoEncoder::GetSharedTexture() noexcept {
  int index = 0;
  SharedVideoTextureFrames* texture_frames = shared_texture_frames_;
  PackedVideoTextureFrame* texture_frame = texture_frames->frames;
  auto texture_frame1 = texture_frame + 1;
  if (texture_frame->stats.timestamp < texture_frame1->stats.timestamp) {
    texture_frame = texture_frame1;
    index = 1;
  }

  bool should_update_shared_texture = false;
  if (!shared_textures_[index].texture) {
    should_update_shared_texture = true;
  } else if (shared_textures_[index].instance_id !=
                 texture_frame->instance_id ||
             shared_textures_[index].texture_id != texture_frame->texture_id) {
    should_update_shared_texture = true;
    ATLTRACE2(atlTraceUtil, 0,
              "SharedTexture[%u] changed: %llu-%llu -> %llu-%llu\n", index,
              shared_textures_[index].instance_id,
              shared_textures_[index].texture_id, texture_frame->instance_id,
              texture_frame->texture_id);
  }

  if (should_update_shared_texture) {
    if (!device_) {
      D3D_DRIVER_TYPE driver_types[]{
          D3D_DRIVER_TYPE_HARDWARE,
          D3D_DRIVER_TYPE_WARP,
          D3D_DRIVER_TYPE_REFERENCE,
      };

      D3D_FEATURE_LEVEL feature_level;
      D3D_FEATURE_LEVEL feature_levels[]{
          D3D_FEATURE_LEVEL_11_1,
      };

      HRESULT hr;
      for (int driver_type_index = 0;
           driver_type_index < ARRAYSIZE(driver_types); driver_type_index++) {
        hr = D3D11CreateDevice(NULL, driver_types[driver_type_index], NULL,
                               D3D11_CREATE_DEVICE_SINGLETHREADED,
                               feature_levels, ARRAYSIZE(feature_levels),
                               D3D11_SDK_VERSION, &device_, &feature_level,
                               &context_);
        if (SUCCEEDED(hr)) {
          break;
        }
      }

      if (FAILED(hr)) {
        ATLTRACE2(atlTraceException, 0, "!D3D11CreateDevice(), #0x%08X\n", hr);
        return hr;
      }
    }

    if (!device1_) {
      HRESULT hr = device_->QueryInterface(IID_PPV_ARGS(&device1_));
      if (FAILED(hr)) {
        ATLTRACE2(atlTraceException, 0,
                  "!QueryInterface(ID3D11Device1), #0x%08X\n", hr);
        return hr;
      }
    }

    std::wstring shared_handle_name =
        std::format(kSharedTextureHandleNameFormat, texture_frame->instance_id,
                    texture_frame->texture_id);
    CComPtr<ID3D11Texture2D> shared_texture;
    HRESULT hr = device1_->OpenSharedResourceByName(
        object_namer_.Get(shared_handle_name).data(), DXGI_SHARED_RESOURCE_READ,
        IID_PPV_ARGS(&shared_texture));
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                "!OpenSharedResourceByName(ID3D11Texture2D), #0x%08X\n", hr);
      return hr;
    }

    D3D11_TEXTURE2D_DESC shared_texture_desc;
    shared_texture->GetDesc(&shared_texture_desc);

    shared_textures_[index].instance_id = texture_frame->instance_id;
    shared_textures_[index].texture_id = texture_frame->texture_id;
    shared_textures_[index].texture = shared_texture;

    bool should_update_staging_texture = false;
    if (!staging_texture_) {
      should_update_staging_texture = true;
    } else if (shared_texture_desc.Width != saved_frame_info_.width ||
               shared_texture_desc.Height != saved_frame_info_.height ||
               shared_texture_desc.Format != saved_frame_info_.format) {
      should_update_staging_texture = true;
    } else {
      D3D11_TEXTURE2D_DESC staging_desc;
      staging_texture_->GetDesc(&staging_desc);
      if (staging_desc.Width != saved_frame_info_.width ||
          staging_desc.Height != saved_frame_info_.height ||
          staging_desc.Format != saved_frame_info_.format) {
        should_update_staging_texture = true;
      }
    }

    if (should_update_staging_texture) {
      D3D11_TEXTURE2D_DESC staging_desc;
      staging_desc = shared_texture_desc;
      staging_desc.BindFlags = 0;
      staging_desc.MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE;
      staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
      staging_desc.Usage = D3D11_USAGE_STAGING;
      CComPtr<ID3D11Texture2D> staging_texture;
      HRESULT hr =
          device1_->CreateTexture2D(&staging_desc, nullptr, &staging_texture);
      if (FAILED(hr)) {
        ATLTRACE2(atlTraceException, 0,
                  "!CreateTexture2D(D3D11_USAGE_STAGING), #0x%08X\n", hr);
        return hr;
      }
      staging_texture_ = staging_texture;

      yuv_frame_data_.resize(4 * saved_frame_info_.width *
                             saved_frame_info_.height);
    }
  }

  context_->CopyResource(staging_texture_, shared_textures_[index].texture);
  return S_OK;
}
