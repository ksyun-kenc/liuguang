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

#include <atlfile.h>

#include <Audioclient.h>
#include <Mmdeviceapi.h>

_CRT_BEGIN_C_HEADER
#include "libavcodec/avcodec.h"
#include "libavutil/samplefmt.h"
_CRT_END_C_HEADER

#include "audio_resampler.h"

#include "regame/shared_mem_info.h"

struct AudioInfo {
  int sample_rate;
  int sample_bits;
  int64_t channel_layout;
  int channels;
  AVSampleFormat sample_format;
};

class ObjectNamer;

class SoundCapturer {
 public:
  SoundCapturer(ObjectNamer& object_namer) : object_namer_(object_namer) {}
  ~SoundCapturer();
  bool Initialize() noexcept;
  void Run();
  void Stop();
  void SetOutputInfo(int64_t channel_layout,
                     AVSampleFormat sample_format,
                     int sample_rate,
                     int frame_size) noexcept {
    out_channel_layout_ = channel_layout;
    out_sample_format_ = sample_format;
    out_sample_rate_ = sample_rate;
    frame_size_ = frame_size;
  }

  int ReadFrame(AVFrame* frame) { return audio_resampler_.ReadFrame(frame); }

 public:
  static HRESULT GetAudioInfo(AudioInfo* format);
  static HRESULT GetAudioClient(IAudioClient** audio_client);

 private:
  HRESULT CaptureThread();

 private:
  ObjectNamer& object_namer_;
  std::thread thread_;
  CHandle stop_event_;
  CAtlFileMapping<regame::SharedAudioFrames> shared_frame_;
  CHandle shared_frame_ready_event_;
  UINT32 frames_ = 0;

  int64_t out_channel_layout_ = 0;
  AVSampleFormat out_sample_format_ = AV_SAMPLE_FMT_NONE;
  int out_sample_rate_ = 0;
  int frame_size_ = 0;
  AudioResampler audio_resampler_;
};
