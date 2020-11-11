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

constexpr int kH264TimeBase = 90000;
constexpr size_t kInitialBufferSize = 0x100000;  // 1MB

enum class NetPacketType : uint8_t { Video = 1, Audio, Ping, Pong };

class EncoderInterface {
 public:
  EncoderInterface() = default;
  ~EncoderInterface() = default;
  virtual NetPacketType GetType() const noexcept = 0;
};
