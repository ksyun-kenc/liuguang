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

namespace regame {
enum class NetPacketType : uint8_t { Audio = 0, Video, Ping, Pong };
constexpr uint8_t kNetPacketCurrentVersion = 0;

#pragma pack(push, 2)
struct NetPacketHeader {
  uint8_t version;
  NetPacketType type;
  uint32_t size;
};
#pragma pack(pop)
}  // namespace regame
