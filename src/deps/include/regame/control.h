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

#pragma pack(push, 1)
enum class ControlType : uint8_t {
  KEYBOARD = 0,
  KEYBOARD_VK,

  MOUSE = 10,

  GAMEPAD_AXIS = 20,
  GAMEPAD_BUTTON,

  // use GAMEPAD instead of JOYSTICK
  JOYSTICK_AXIS = 30,
  JOYSTICK_BALL,
  JOYSTICK_BUTTON,
  JOYSTICK_HAT,

  PING = 40,
  PONG
};

enum class ControlButtonState : uint8_t { Released = 0, Pressed = 1 };

struct ControlBase {
  ControlType type;
  uint32_t timestamp;
};

struct ControlPing : public ControlBase {};

struct ControlKeyboard : public ControlBase {
  uint16_t key_code;
  ControlButtonState state;
};

struct ControlJoystickAxis : public ControlBase {
  int32_t which;
  uint8_t axis;
  uint16_t value; // -32768(0x8000) to 32767(0x7fff)
};

struct ControlJoystickBall : public ControlBase {
  int32_t which;
  uint8_t ball;
  int16_t x;
  int16_t y;
};

struct ControlJoystickButton : public ControlBase {
  int32_t which;
  uint8_t button;
  ControlButtonState state;
};

struct ControlJoystickHat : public ControlBase {
  int32_t which;
  uint8_t hat;
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
};
#pragma pack(pop)
