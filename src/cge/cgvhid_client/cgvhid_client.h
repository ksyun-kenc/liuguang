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

#include "cgvhid.h"

struct MouseState {
  int display_width;
  int display_height;
  POINT postion;
  std::uint8_t buttons;
};

struct KeyboardState {
  std::uint8_t modifiers;
  std::uint8_t key_codes[KEYBD_MAX_KEY_COUNT];
};

// VhidGamepadInReport without id
struct GamepadState {
  std::uint16_t x;
  std::uint16_t y;
  std::uint16_t z;
  std::uint16_t rx;
  std::uint16_t ry;
  std::uint16_t rz;
  std::uint8_t hat;
  std::uint16_t buttons;
};

class CgvhidClient {
 public:
  CgvhidClient() = default;
  ~CgvhidClient() = default;

  static std::uint8_t VkToScancode(std::uint8_t vk) noexcept;

  void Init(int display_width, int display_height) noexcept;

  int KeyboardReset() noexcept;
  int KeyboardPress(std::uint8_t scancode) noexcept;
  int KeyboardRelease(std::uint8_t scancode) noexcept;

  int KeyboardVkPress(std::uint8_t vk) noexcept;
  int KeyboardVkRelease(std::uint8_t vk) noexcept;

  int MouseReset() noexcept;
  int AbsoluteMouseButtonPress(CgvhidMouseButton button,
                               std::uint16_t x,
                               std::uint16_t y) noexcept;
  int AbsoluteMouseButtonRelease(CgvhidMouseButton button,
                                 std::uint16_t x,
                                 std::uint16_t y) noexcept;
  int AbsoluteMouseMove(std::uint16_t x, std::uint16_t y) noexcept;
  int AbsoluteMouseWheel(std::int8_t hwheel, std::int8_t vwheel) noexcept;

  int RelativeMouseButtonPress(CgvhidMouseButton button) noexcept;
  int RelativeMouseButtonRelease(CgvhidMouseButton button) noexcept;
  int RelativeMouseMove(std::int16_t x, std::int16_t y) noexcept;
  int RelativeMouseWheel(std::int8_t hwheel, std::int8_t vwheel) noexcept;

 private:
  int KeyboardModifierPress(std::uint8_t key) noexcept;
  int KeyboardModifierRelease(std::uint8_t key) noexcept;

 private:
  Cgvhid cgvhid_;
  KeyboardState keyboard_state_;
  MouseState mouse_state_;
};
