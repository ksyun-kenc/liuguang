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

#include "regame/protocol.h"

constexpr int kH264TimeBase = 90000;
constexpr size_t kInitialBufferSize = 0x100000;  // 1MB

class Encoder {
 public:
  virtual regame::NetPacketType GetType() const noexcept = 0;

  const std::string& GetHeader() const noexcept { return header_; }

  void SaveHeader(const uint8_t* buffer, int size) noexcept {
    if (header_.empty()) {
      header_.resize(sizeof(regame::NetPacketHeader));
      auto header = reinterpret_cast<regame::NetPacketHeader*>(header_.data());
      header->version = regame::kNetPacketCurrentVersion;
      header->type = GetType();
      header->size = htonl(size);
    } else {
      auto header = reinterpret_cast<regame::NetPacketHeader*>(header_.data());
      header->size = htonl(ntohl(header->size) + size);
    }
    header_.append(reinterpret_cast<const char*>(buffer), size);
  }

 protected:
  Encoder() = default;
  ~Encoder() = default;

  void FreeHeader() noexcept { header_.clear(); }

 private:
  std::string header_;
};
