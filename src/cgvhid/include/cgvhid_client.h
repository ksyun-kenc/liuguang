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
#include "vhid_common.h"

struct MouseState {
  int display_width;
  int display_height;
  POINT postion;
  uint8_t buttons;
};

struct KeyboardState {
  uint8_t modifiers;
  uint8_t key_codes[KEYBD_MAX_KEY_COUNT];
};

struct GamepadState {
  uint16_t buttons;
  uint16_t x;
  uint16_t y;
  uint16_t z;
  uint16_t rx;
  uint16_t ry;
  uint16_t rz;
  uint8_t hat;
};

class CgvhidClient {
 public:
  CgvhidClient() = default;
  ~CgvhidClient() = default;

  void Init(int display_width, int display_height) noexcept;

  int KeyboardReset() noexcept;
  int KeyboardPress(uint64_t vk) noexcept;
  int KeyboardRelease(uint64_t vk) noexcept;

 private:
  int KeyboardModifierPress(uint8_t key) noexcept;
  int KeyboardModifierRelease(uint8_t key) noexcept;

 private:
  Cgvhid cgvhid_;
  KeyboardState keyboard_state_;
};
