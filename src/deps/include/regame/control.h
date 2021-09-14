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

constexpr std::uint8_t kCurrentControlVersion = 0;

#pragma pack(push, 2)
enum class ControlType : std::uint8_t {
  kKeyboard = 0,
  kKeyboardVk,

  kAbsoluteMouseMove = 10,
  kAbsoluteMouseButton,
  kAbsoluteMouseWheel,
  kRelativeMouseMove,
  kRelativeMouseButton,
  kRelativeMouseWheel,

  kGamepadAxis = 20,
  kGamepadButton,

  kJoystickAxis = 30,  // use GAMEPAD instead of JOYSTICK
  kJoystickBall,
  kJoystickButton,
  kJoystickHat,

  kPing = 40,
  kPong
};

enum class ControlButtonState : std::uint8_t { Released = 0, Pressed = 1 };

struct ControlBase {
  std::uint8_t version;
  ControlType type;
  std::uint32_t timestamp;
};

struct ControlPing : public ControlBase {};

struct ControlKeyboard : public ControlBase {
  std::uint16_t key_code;
  ControlButtonState state;
};

struct ControlAbsoluteMouseButton : public ControlBase {
  std::uint8_t button;
  ControlButtonState state;
  std::uint16_t x;
  std::uint16_t y;
};

struct ControlAbsoluteMouseMove : public ControlBase {
  std::uint16_t x;
  std::uint16_t y;
};

struct ControlAbsoluteMouseWheel : public ControlBase {
  std::uint16_t x;
  std::uint16_t y;
};

struct ControlRelativeMouseButton : public ControlBase {
  std::uint8_t button;
  ControlButtonState state;
};

struct ControlRelativeMouseMove : public ControlBase {
  std::int8_t x;
  std::int8_t y;
};

struct ControlRelativeMouseWheel : public ControlBase {
  std::int8_t x;
  std::int8_t y;
};

struct ControlJoystickAxis : public ControlBase {
  std::uint8_t which;
  std::uint8_t axis;
  std::uint16_t value;  // -32768(0x8000) to 32767(0x7fff)
};

struct ControlJoystickBall : public ControlBase {
  std::uint8_t which;
  std::uint8_t ball;
  std::int16_t x;
  std::int16_t y;
};

struct ControlJoystickButton : public ControlBase {
  std::uint8_t which;
  std::uint8_t button;
  ControlButtonState state;
};

struct ControlJoystickHat : public ControlBase {
  std::uint8_t which;
  std::uint8_t hat; // CgvhidGamepadHat
};

struct ControlGamepadAxis : public ControlJoystickAxis {};

struct ControlGamepadButton : public ControlJoystickButton {};

union ControlElement {
  ControlBase base;
  ControlPing ping;
  ControlKeyboard keyboard;
  ControlJoystickAxis joystick_axis;
  ControlJoystickButton joystick_button;
  ControlJoystickHat joystick_hat;
  ControlGamepadAxis gamepad_axis;
  ControlGamepadButton gamepad_button;
  ControlAbsoluteMouseButton absolute_mouse_button;
  ControlAbsoluteMouseMove absolute_mouse_move;
  ControlAbsoluteMouseWheel absolute_mouse_wheel;
  ControlRelativeMouseButton relative_mouse_button;
  ControlRelativeMouseMove relative_mouse_move;
  ControlRelativeMouseWheel relative_mouse_wheel;
};
#pragma pack(pop)

}  // namespace regame
