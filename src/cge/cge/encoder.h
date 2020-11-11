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

#include "audio_encoder.h"
#include "video_encoder.h"

class Encoder {
 public:
  Encoder() = default;
  ~Encoder() = default;

  bool Init(std::string audio_codec,
            uint64_t audio_bitrate,
            bool enable_nvenc,
            uint64_t video_bitrate,
            int video_gop,
            std::string video_preset,
            uint32_t video_quality) noexcept;
  void Run();
  void Stop();

 private:
  AudioEncoder audio_encoder_;
  VideoEncoder video_encoder_;
};
