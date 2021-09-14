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

enum class NetPacketType : uint8_t {
  kLogin = 0,
  kAudio,
  kVideo,
  kPing,
  kPong,
  kResetAudio,
  kResetVideo
};

enum class VerificationType : uint8_t { Code = 0, SM3 };

constexpr uint8_t kNetPacketCurrentVersion = 0;
constexpr uint8_t kMaxUsernameSize = 32;
constexpr uint8_t kMaxVerificationSize = 32;
constexpr uint8_t kMinVerificationSize = 6;

#pragma pack(push, 2)
struct NetPacketHeader {
  uint8_t version;
  NetPacketType type;
  uint32_t size;
};

struct Login {
  char username[kMaxUsernameSize];  // Mobile phone or Email(max 256, but why
                                    // that long?)
  VerificationType verification_type;
  uint8_t verification_size;  // <= kMaxVerificationSize
  char verification_data[kMaxVerificationSize];
};

struct NetPacketLogin {
  NetPacketHeader header;
  Login login;
};

struct LoginResult {
  int error_code;
  uint32_t audio_codec;
  uint32_t video_codec;
};

struct NetPacketLoginResult {
  NetPacketHeader header;
  LoginResult login_result;
};
#pragma pack(pop)

}  // namespace regame
