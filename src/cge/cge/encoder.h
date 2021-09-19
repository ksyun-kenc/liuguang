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
  AVCodecID GetCodecID() const noexcept { return codec_id_; }
  const std::string& GetHeader() const noexcept { return header_; }
  regame::ServerAction GetServerAction() const noexcept { return action_; }

  void SaveHeader(std::span<std::uint8_t> buffer) noexcept {
    if (header_.empty()) {
      header_.resize(sizeof(regame::PackageHead) +
                     sizeof(regame::ServerPacket));
      auto head = reinterpret_cast<regame::PackageHead*>(header_.data());
      head->size =
          htonl(static_cast<int>(sizeof(regame::ServerPacket) + buffer.size()));
      auto packet = reinterpret_cast<regame::ServerPacket*>(head + 1);
      packet->action = GetServerAction();
    } else {
      auto head = reinterpret_cast<regame::PackageHead*>(header_.data());
      head->size = htonl(ntohl(head->size) + static_cast<int>(buffer.size()));
    }
    header_.append(reinterpret_cast<const char*>(buffer.data()), buffer.size());
  }

 protected:
  Encoder(regame::ServerAction action) : action_(action) {}
  ~Encoder() = default;

  void FreeHeader() noexcept { header_.clear(); }
  void SetCodecID(AVCodecID codec_id) noexcept { codec_id_ = codec_id; }

 private:
  AVCodecID codec_id_ = AV_CODEC_ID_NONE;
  std::string header_;
  regame::ServerAction action_;
};
