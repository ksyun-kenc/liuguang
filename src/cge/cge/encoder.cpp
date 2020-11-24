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

#include "encoder.h"

#include "app.hpp"

extern App g_app;

bool Encoder::Init(std::string audio_codec,
                   uint64_t audio_bitrate,
                   bool enable_nvenc,
                   uint64_t video_bitrate,
                   std::string video_codec,
                   int video_gop,
                   std::string video_preset,
                   uint32_t video_quality) noexcept {
  bool audio = audio_encoder_.Init(std::move(audio_codec), audio_bitrate);
  bool video =
      video_encoder_.Init(enable_nvenc, video_bitrate, video_codec, video_gop,
                          std::move(video_preset), video_quality);
  if (!audio && !video) {
    return false;
  }
  return true;
}

void Encoder::Run() {
  int error_code = audio_encoder_.Run();
  if (error_code < 0) {
    return;
  }
  error_code = video_encoder_.Run();
  if (error_code < 0) {
    return;
  }
}

void Encoder::Stop() {
  audio_encoder_.Stop();
  video_encoder_.Stop();
}