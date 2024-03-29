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

constexpr std::uint8_t kProtocolVersion = 1;
constexpr std::uint8_t kMinUsernameSize = 3;
constexpr std::uint8_t kMaxUsernameSize = 32;
constexpr std::uint8_t kMinVerificationSize = 6;
constexpr std::uint8_t kMaxVerificationSize = 32;

enum class ClientAction : std::uint8_t {
  kLogin = 0,
  kControl,
  kPing,
  kPong,
};

enum class ServerAction : std::uint8_t {
  kLoginResult = 0,
  kControl,
  kPing,
  kPong,
  kAudio,
  kVideo,
  kResetAudio,
  kResetVideo,
};

enum class VerificationType : std::uint8_t { Code = 0, SM3 };

#pragma pack(push, 2)
struct PackageHead {
  // Package = std::uint32_t size + std::byte data[size]
  // Try to support Raw TCP, WebSocket, KCP
  // Raw TCP needs "size" to distinguish the boundaries
  std::uint32_t size;
};

struct ClientPacketHead {
  ClientAction action;
};

struct ClientLogin {
  ClientPacketHead head;
  std::uint8_t protocol_version;
  char username[kMaxUsernameSize];  // Mobile phone or Email(max 256, but why
                                    // that long?)
  VerificationType verification_type;
  std::uint8_t verification_size;  // <= kMaxVerificationSize
  char verification_data[kMaxVerificationSize];
};
static_assert((sizeof(ClientLogin) & 1) == 0);

#pragma region ClientControl
enum class ControlType : std::uint8_t {
  kKeyboard = 0,
  kKeyboardVk,

  kMouseMove = 10,
  kMouseButton,
  kMouseWheel,
  kRelativeMouseMove,
  kRelativeMouseButton,
  kRelativeMouseWheel,

  kGamepadAxis = 20,
  kGamepadButton,

  kJoystickAxis = 30,  // use GAMEPAD instead of JOYSTICK
  kJoystickBall,
  kJoystickButton,
  kJoystickHat,
};

enum class ButtonState : std::uint8_t { Released = 0, Pressed = 1 };

struct ClientControl {
  ClientPacketHead head;
  ControlType type;  // 2-aligned
  std::uint32_t timestamp;
};
static_assert((sizeof(ClientControl) & 1) == 0);

struct ClientKeyboard {
  ClientControl control;
  std::uint16_t key_code;  // SDL scancode or VK
  ButtonState state;
  std::byte reserved;  // padding
};
static_assert((sizeof(ClientKeyboard) & 1) == 0);

struct ClientMouseButton {
  ClientControl control;
  std::uint8_t button;
  ButtonState state;
  std::uint16_t x;
  std::uint16_t y;
};
static_assert((sizeof(ClientMouseButton) & 1) == 0);

struct ClientMouseMove {
  ClientControl control;
  std::uint16_t x;
  std::uint16_t y;
};
static_assert((sizeof(ClientMouseMove) & 1) == 0);

struct ClientMouseWheel {
  ClientControl control;
  std::int8_t x;
  std::int8_t y;
};
static_assert((sizeof(ClientMouseWheel) & 1) == 0);

struct ClientRelativeMouseButton {
  ClientControl control;
  std::uint8_t button;
  ButtonState state;
};
static_assert((sizeof(ClientRelativeMouseButton) & 1) == 0);

struct ClientRelativeMouseMove {
  ClientControl control;
  std::int16_t x;
  std::int16_t y;
};
static_assert((sizeof(ClientRelativeMouseMove) & 1) == 0);

struct ClientRelativeMouseWheel {
  ClientControl control;
  std::int8_t x;
  std::int8_t y;
};
static_assert((sizeof(ClientRelativeMouseWheel) & 1) == 0);

struct ClientJoystickAxis {
  ClientControl control;
  std::uint8_t which;
  std::uint8_t axis;
  std::uint16_t value;  // -32768(0x8000) to 32767(0x7fff)
};
static_assert((sizeof(ClientJoystickAxis) & 1) == 0);

struct ClientJoystickBall {
  ClientControl control;
  std::uint8_t which;
  std::uint8_t ball;
  std::int16_t x;
  std::int16_t y;
};
static_assert((sizeof(ClientJoystickBall) & 1) == 0);

struct ClientJoystickButton {
  ClientControl control;
  std::uint8_t which;
  std::uint8_t button;
  ButtonState state;
};
static_assert((sizeof(ClientJoystickButton) & 1) == 0);

struct ClientJoystickHat {
  ClientControl control;
  std::uint8_t which;
  std::uint8_t hat;  // CgvhidGamepadHat
};
static_assert((sizeof(ClientJoystickHat) & 1) == 0);

struct ClientGamepadAxis : public ClientJoystickAxis {};

struct ClientGamepadButton : public ClientJoystickButton {};
#pragma endregion()

struct ServerPacketHead {
  ServerAction action;
};

enum WorkMode : std::uint16_t {
  kDesktop = 1,
};

struct ServerLoginResult {
  // version >= 0
  ServerPacketHead head;
  std::uint8_t protocol_version;
  std::int32_t error_code;
  std::uint32_t audio_codec;
  std::uint32_t video_codec;
  // version >= 1
  WorkMode work_modes;
};
static_assert((sizeof(ServerLoginResult) & 1) == 0);
#pragma pack(pop)

}  // namespace regame
