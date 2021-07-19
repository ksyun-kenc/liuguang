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

#include "audio_encoder.h"

#include "engine.h"
#include "sound_capturer.h"

extern App g_app;

const int kTargetChannels = 2;
const int64_t kTargetChannelLayout = AV_CH_LAYOUT_STEREO;

bool AudioEncoder::Init(std::string codec_name, uint64_t bitrate) noexcept {
  codec_name_ = std::move(codec_name);
  bitrate_ = bitrate;

  HANDLE ev =
      CreateEvent(g_app.SA(), TRUE, FALSE, kAudioStartedEventName.data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return false;
  }
  started_event_.Attach(ev);

  ev = CreateEvent(g_app.SA(), TRUE, FALSE, kAudioStoppedEventName.data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return false;
  }
  stop_event_.Attach(ev);

  ev = CreateEvent(g_app.SA(), FALSE, FALSE,
                   kSharedAudioFrameReadyEventName.data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return false;
  }
  shared_frame_ready_event_.Attach(ev);
  return true;
}

int AudioEncoder::Run() {
  ResetEvent(stop_event_);
  thread_ = std::thread(&AudioEncoder::EncodingThread, this);
  if (nullptr == thread_.native_handle()) {
    APP_ERROR() << "Create thread failed with " << GetLastError() << ".\n";
    return -1;
  }
  return 0;
}

void AudioEncoder::Stop() {
  Free(true);
}

int AudioEncoder::EncodingThread() {
  BOOST_SCOPE_EXIT_ALL(&) { Free(false); };

  int error_code = -1;
  AVDictionary* opts = nullptr;
  if (codec_name_ == "aac") {
    SetCodecID(AV_CODEC_ID_AAC);
    error_code = avformat_alloc_output_context2(&format_context_, nullptr,
                                                "mp4", nullptr);
    av_dict_set(&opts, "movflags",
                "frag_every_frame+dash+delay_moov+skip_sidx+skip_trailer", 0);
  } else if (codec_name_ == "libopus" || codec_name_ == "opus") {
    SetCodecID(AV_CODEC_ID_OPUS);
    error_code = avformat_alloc_output_context2(&format_context_, nullptr,
                                                "opus", nullptr);
  } else {
    assert(false);
  }
  if (error_code < 0) {
    return error_code;
  }

  sound_capturer_.GetAudioInfo(&source_audio_info_);

  AVCodec* codec = nullptr;
  error_code = AddStream(codec);
  if (error_code < 0) {
    return error_code;
  }

  error_code = Open(codec, &opts);
  av_dict_free(&opts);
  if (error_code < 0) {
    return error_code;
  }

  size_t frame_bytes = codec_context_->frame_size *
                       (codec_context_->bits_per_raw_sample + 7) / 8 *
                       codec_context_->channels;
  size_t shared_mem_size = sizeof(SharedAudioFrames) +
                           sizeof(PackedAudioFrame) +
                           frame_bytes * kNumberOfSharedFrames;
  // UMU: In Pro version, may get audio from other process.
  HRESULT hr = shared_frames_.MapSharedMem(
      shared_mem_size, kSharedAudioFrameFileMappingName.data(), nullptr,
      g_app.SA());
  if (FAILED(hr)) {
    APP_ERROR() << "MapSharedMem() failed with 0x" << std::hex << hr << ".\n";
    error_code = -1;
    return error_code;
  }
  SharedAudioFrames* saf = shared_frames_;
  strcpy_s(saf->codec_name, codec_name_.data());
  saf->channels = codec_context_->channels;
  saf->frame_size = codec_context_->frame_size;
  saf->sample_bits = codec_context_->bits_per_raw_sample;
  saf->sample_format = codec_context_->sample_fmt;
  saf->frame0.timestamp = 0;

  SetEvent(started_event_);
  error_code = Encode();
  if (error_code < 0) {
    APP_ERROR() << "Encode() failed with " << error_code << ".\n";
    return error_code;
  }
  return error_code;
}

void AudioEncoder::Free(bool wait_thread) {
  FreeHeader();
  sound_capturer_.Stop();

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

int AudioEncoder::AddStream(AVCodec*& codec) {
  assert(!codec_name_.empty());
  assert(nullptr == codec_context_);
  assert(source_audio_info_.sample_bits > 0);
  assert(source_audio_info_.sample_rate > 0);

  codec = avcodec_find_encoder_by_name(codec_name_.data());
  if (nullptr == codec) {
    APP_ERROR() << "Could not find encoder for " << codec_name_ << ".\n";
    return -1;
  }
  next_pts_ = 0;
  stream_ = avformat_new_stream(format_context_, nullptr);
  if (nullptr == stream_) {
    APP_ERROR() << "Could not allocate stream for " << codec_name_ << ".\n";
    return -1;
  }
  stream_->id = format_context_->nb_streams - 1;

  codec_context_ = avcodec_alloc_context3(codec);
  if (nullptr == codec_context_) {
    APP_ERROR() << "Could not allocate codec context for " << codec_name_
                << ".\n";
    return -1;
  }

  codec_context_->thread_count = 1;
  if (nullptr == codec->sample_fmts) {
    APP_ERROR() << "Could not find suitable sample format for " << codec_name_
                << ".\n";
    return -1;
  }

  codec_context_->sample_fmt = codec->sample_fmts[0];
  for (int i = 1; AV_SAMPLE_FMT_NONE != codec->sample_fmts[i]; ++i) {
    if (codec->sample_fmts[i] == source_audio_info_.sample_format) {
      codec_context_->sample_fmt = codec->sample_fmts[i];
    }
  }
  // codec_context_->sample_fmt = AV_SAMPLE_FMT_S16;
  codec_context_->bits_per_raw_sample = source_audio_info_.sample_bits;
  codec_context_->bit_rate = bitrate_;
  codec_context_->sample_rate = source_audio_info_.sample_rate;
  const int* ssr = codec->supported_samplerates;
  if (nullptr != ssr) {
    int min_diff = -1;
    for (int i = 0; 0 != ssr[i]; ++i) {
      if (-1 == min_diff) {
        min_diff = std::abs(source_audio_info_.sample_rate - ssr[i]);
        codec_context_->sample_rate = ssr[i];
      } else {
        int diff = std::abs(source_audio_info_.sample_rate - ssr[i]);
        if (diff < min_diff) {
          min_diff = diff;
          codec_context_->sample_rate = ssr[i];
        }
      }
    }
  }

  // UMU: on MBP there are 4 channels, which may be unsupported by the encoder.
  // codec_context_->channels = source_audio_info_.channels;
  // codec_context_->channel_layout =
  //    av_get_default_channel_layout(codec_context_->channels);
  // if (codec_context_->channel_layout == 0) {
  //  std::cerr << "invaild channels " << source_audio_info_.channels << ".\n";
  //  return -1;
  // }
  codec_context_->channels = kTargetChannels;
  codec_context_->channel_layout = kTargetChannelLayout;
  if (codec_name_ == "opus") {
    codec_context_->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
  }

  stream_->time_base = {1, codec_context_->sample_rate};

  return 0;
}

int AudioEncoder::Open(AVCodec* codec, AVDictionary** opts) {
  assert(nullptr != codec_context_);
  assert(nullptr != stream_);

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

  sound_capturer_.SetOutputInfo(
      codec_context_->channel_layout, codec_context_->sample_fmt,
      codec_context_->sample_rate, codec_context_->frame_size);

#if _DEBUG
  if (source_audio_info_.channel_layout != codec_context_->channel_layout) {
    APP_TRACE() << "resample channel layout "
                << source_audio_info_.channel_layout << " to "
                << codec_context_->channel_layout << ".\n";
  }
  if (source_audio_info_.sample_format != codec_context_->sample_fmt) {
    APP_TRACE() << "resample format " << source_audio_info_.sample_format
                << " to " << codec_context_->sample_fmt << ".\n";
  }
  if (source_audio_info_.sample_rate != codec_context_->sample_rate) {
    APP_TRACE() << "resample rate " << source_audio_info_.sample_rate << " to "
                << codec_context_->sample_rate << ".\n";
  }
#endif

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

int AudioEncoder::InitFrame(AVFrame*& frame) const noexcept {
  assert(nullptr != codec_context_);
  assert(0 != codec_context_->frame_size);

  frame = av_frame_alloc();
  if (nullptr == frame) {
    return AVERROR(ENOMEM);
  }

  frame->nb_samples = codec_context_->frame_size;
  frame->format = codec_context_->sample_fmt;
  frame->sample_rate = codec_context_->sample_rate;
  frame->channel_layout = codec_context_->channel_layout;

  int error_code = av_frame_get_buffer(frame, 0);
  if (error_code < 0) {
    av_frame_free(&frame);
    ATLTRACE2(atlTraceException, 0, "!av_frame_get_buffer(), #%d, %s\n",
              error_code, GetAvErrorText(error_code));
  }
  return error_code;
}

int AudioEncoder::Encode() {
#if _DEBUG
  APP_TRACE() << __func__ << "+\n";
#endif
  sound_capturer_.Run();
  BOOST_SCOPE_EXIT_ALL(&) { sound_capturer_.Stop(); };

  AVFrame* frame = nullptr;
  int error_code = InitFrame(frame);
  if (error_code < 0) {
    APP_ERROR() << "Init frame failed with " << error_code << ".\n";
    return error_code;
  }
  BOOST_SCOPE_EXIT_ALL(&) { av_frame_free(&frame); };

  HANDLE events[] = {stop_event_, shared_frame_ready_event_};
  for (;;) {
    DWORD wait =
        WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
    if (WAIT_OBJECT_0 == wait) {
      ATLTRACE2(atlTraceUtil, 0, "%s: stopping.\n", __func__);
      break;
    } else if (WAIT_OBJECT_0 + 1 != wait) {
      int error_code = GetLastError();
      APP_WARNING() << "Unexpected WaitForMultipleObjects() return " << wait
                    << ", error code " << error_code << ".\n";
      return error_code;
    }

    for (;;) {
      int error_code = sound_capturer_.ReadFrame(frame);
      if (error_code < 0) {
        if (AVERROR(EAGAIN) != error_code) {
          ATLTRACE2(atlTraceException, 0, "%s: !ReadFrame(), #%d\n", __func__,
                    error_code);
        }
        break;
      }
      ATLTRACE2(atlTraceUtil, 0, "%s: Read a frame.\n", __func__);

      frame->pts = next_pts_;
      next_pts_ += frame->nb_samples;
      error_code = avcodec_send_frame(codec_context_, frame);
      if (0 != error_code) {
        ATLTRACE2(atlTraceException, 0, "%s: !avcodec_send_frame(), #%d, %s.\n",
                  __func__, error_code, GetAvErrorText(error_code));
        continue;
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
    }    // end of while
  }      // end of for

#if _DEBUG
  APP_TRACE() << __func__ << "-\n";
#endif
  return 0;
}
