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

#pragma once

#include "ffmpeg.h"

class AudioResampler {
 public:
  AudioResampler() noexcept {}
  ~AudioResampler() noexcept { Free(); }

  int Initialize(int64_t in_channel_layout,
                 enum AVSampleFormat in_sample_format,
                 int in_sample_rate,
                 int64_t out_channel_layout,
                 enum AVSampleFormat out_sample_format,
                 int out_sample_rate,
                 int frame_size) noexcept;
  void Free() noexcept;
  int Store(const uint8_t* in, int in_samples, int* sample_count = nullptr);
  int InitializeFrame(AVFrame*& frame, int frame_size) const noexcept;
  int ReadFrame(AVFrame* frame);

 private:
  int in_sample_rate_ = 0;

  int64_t out_channel_layout_ = 0;
  AVSampleFormat out_sample_format_ = AV_SAMPLE_FMT_NONE;
  int out_sample_rate_ = 0;
  int frame_size_ = 0;

  SwrContext* resampler_context_ = nullptr;
  AVAudioFifo* fifo_ = nullptr;

  std::mutex mutex_;

  boost::pool<> uint8_ptr_pool_{sizeof(uint8_t*)};
};
