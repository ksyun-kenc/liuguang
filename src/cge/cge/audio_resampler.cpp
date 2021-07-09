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

#include "audio_resampler.h"

int AudioResampler::Init(int64_t in_channel_layout,
                         AVSampleFormat in_sample_format,
                         int in_sample_rate,
                         int64_t out_channel_layout,
                         AVSampleFormat out_sample_format,
                         int out_sample_rate,
                         int frame_size) noexcept {
  assert(nullptr == resampler_context_);
  assert(nullptr == fifo_);

  std::lock_guard<std::mutex> lock(mutex_);

  out_channel_layout_ = out_channel_layout;
  out_sample_format_ = out_sample_format;
  out_sample_rate_ = out_sample_rate;
  frame_size_ = frame_size;

  if (in_channel_layout != out_channel_layout ||
      in_sample_format != out_sample_format ||
      in_sample_rate != out_sample_rate) {
    in_sample_rate_ = in_sample_rate;

    resampler_context_ = swr_alloc_set_opts(
        nullptr, out_channel_layout, out_sample_format, out_sample_rate,
        in_channel_layout, in_sample_format, in_sample_rate, 0, nullptr);
    if (nullptr == resampler_context_) {
      ATLTRACE2(atlTraceException, 0, "!swr_alloc_set_opts()\n");
      return AVERROR(ENOMEM);
    }

    int error = swr_init(resampler_context_);
    if (error < 0) {
      ATLTRACE2(atlTraceException, 0, "!swr_init(), #%d, %s\n", error,
                GetAvErrorText(error));
      swr_free(&resampler_context_);
    } else {
      ATLTRACE2(
          atlTraceUtil, 0,
          "Resample: channel_layout 0x%llx -> 0x%llx, sample_fmt %d -> %d, "
          "sample_rate %d -> %d\n",
          in_channel_layout, out_channel_layout, in_sample_format,
          out_sample_format, in_sample_rate, out_sample_rate);
    }
  }

  fifo_ = av_audio_fifo_alloc(
      out_sample_format_,
      av_get_channel_layout_nb_channels(out_channel_layout_), frame_size_ * 3);
  if (nullptr == fifo_) {
    ATLTRACE2(atlTraceException, 0, "!av_audio_fifo_alloc()\n");
    return AVERROR(ENOMEM);
  }
  return 0;
}

void AudioResampler::Free() noexcept {
  std::lock_guard<std::mutex> lock(mutex_);
  if (nullptr != fifo_) {
    av_audio_fifo_reset(fifo_);
    av_audio_fifo_free(fifo_);
    fifo_ = nullptr;
  }
  swr_free(&resampler_context_);
  uint8_ptr_pool_.purge_memory();
}

int AudioResampler::Store(const uint8_t* in,
                          int in_samples,
                          int* sample_count) {
  assert(nullptr != fifo_);
  std::lock_guard<std::mutex> lock(mutex_);
  if (nullptr == fifo_) {
    return 0;
  }

  int new_fifo_size = 0;

  if (nullptr == resampler_context_) {
    new_fifo_size = av_audio_fifo_size(fifo_) + in_samples;
    int error = av_audio_fifo_realloc(fifo_, new_fifo_size);
    if (0 != error) {
      ATLTRACE2(atlTraceException, 0, "!av_audio_fifo_realloc(), #%d, %s\n",
                error, GetAvErrorText(error));
      return error;
    }
    if (av_audio_fifo_write(
            fifo_, reinterpret_cast<void**>(const_cast<uint8_t**>(&in)),
            in_samples) < in_samples) {
      return AVERROR_EXIT;
    }
  } else {
    int channel_count = av_get_channel_layout_nb_channels(out_channel_layout_);
    auto converted_samples =
        static_cast<uint8_t**>(uint8_ptr_pool_.ordered_malloc(channel_count));
    if (nullptr == converted_samples) {
      ATLTRACE2(atlTraceException, 0, "new failed!\n");
      return AVERROR(ENOMEM);
    }
    BOOST_SCOPE_EXIT_ALL(&) {
      uint8_ptr_pool_.ordered_free(converted_samples, channel_count);
    };

    // a * b / c
    int out_size = static_cast<int>(av_rescale_rnd(
        swr_get_delay(resampler_context_, out_sample_rate_) + in_samples,
        out_sample_rate_, in_sample_rate_, AV_ROUND_UP));
    int error = av_samples_alloc(converted_samples, nullptr, channel_count,
                                 out_size, out_sample_format_, 0);
    if (error < 0) {
      ATLTRACE2(atlTraceException, 0, "!av_samples_alloc(), #%d, %s\n", error,
                GetAvErrorText(error));
      return error;
    }
    BOOST_SCOPE_EXIT_ALL(&) { av_freep(&converted_samples[0]); };

    error = swr_convert(resampler_context_, converted_samples, out_size, &in,
                        in_samples);
    if (error < 0) {
      ATLTRACE2(atlTraceException, 0, "!swr_convert(), #%d, %s\n", error,
                GetAvErrorText(error));
      return error;
    }
    int out_samples = error;

    new_fifo_size = av_audio_fifo_size(fifo_) + out_samples;
    error = av_audio_fifo_realloc(fifo_, new_fifo_size);
    if (0 != error) {
      ATLTRACE2(atlTraceException, 0, "!av_audio_fifo_realloc(), #%d, %s\n",
                error, GetAvErrorText(error));
      return error;
    }
    if (av_audio_fifo_write(fifo_, reinterpret_cast<void**>(converted_samples),
                            out_samples) < out_samples) {
      return AVERROR_EXIT;
    }

    error = swr_convert(resampler_context_, converted_samples, out_size,
                        nullptr, 0);
    if (error < 0) {
      ATLTRACE2(atlTraceException, 0, "!swr_convert(), #%d, %s\n", error,
                GetAvErrorText(error));
      return 0;
    }
    if (error > 0) {
      out_samples = error;
      new_fifo_size = av_audio_fifo_size(fifo_) + out_samples;
      error = av_audio_fifo_realloc(fifo_, new_fifo_size);
      if (0 != error) {
        ATLTRACE2(atlTraceException, 0, "!av_audio_fifo_realloc(2), #%d, %s\n",
                  error, GetAvErrorText(error));
        return error;
      }
      if (av_audio_fifo_write(fifo_,
                              reinterpret_cast<void**>(converted_samples),
                              out_samples) < out_samples) {
        return AVERROR_EXIT;
      }
    }
  }
  if (nullptr != sample_count) {
    *sample_count = new_fifo_size;
    ATLTRACE2(atlTraceUtil, 0, "%d => %d\n", in_samples, new_fifo_size);
  }
  return 0;
}

int AudioResampler::InitFrame(AVFrame*& frame, int frame_size) const noexcept {
  frame = av_frame_alloc();
  if (nullptr == frame) {
    return AVERROR(ENOMEM);
  }

  frame->nb_samples = frame_size;
  frame->format = out_sample_format_;
  frame->sample_rate = out_sample_rate_;
  frame->channel_layout = out_channel_layout_;

  if (0 == frame_size) {
    return 0;
  }

  int error = av_frame_get_buffer(frame, 0);
  if (error < 0) {
    av_frame_free(&frame);
    ATLTRACE2(atlTraceException, 0, "!av_frame_get_buffer(), #%d, %s\n", error,
              GetAvErrorText(error));
  }
  return error;
}

int AudioResampler::ReadFrame(AVFrame* frame) {
  assert(nullptr != fifo_);
  std::lock_guard<std::mutex> lock(mutex_);
  if (av_audio_fifo_size(fifo_) < frame_size_) {
    return AVERROR(EAGAIN);
  }

  int read_size = av_audio_fifo_read(fifo_, (void**)frame->data, frame_size_);
  if (read_size < frame_size_) {
    ATLTRACE2(atlTraceException, 0, "av_audio_fifo_read(%d) = %d\n",
              frame_size_, read_size);
    return AVERROR_EXIT;
  }

  return 0;
}