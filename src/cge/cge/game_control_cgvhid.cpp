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

#include "pch.h"

#include "game_control.h"

#include "app.hpp"
#include "sdl.hpp"

namespace {
// SDL scancode to HID scancode
std::uint8_t SdlScancodeToHid(std::uint16_t sdl_scancode) {
  switch (sdl_scancode) {
    case SDL_SCANCODE_MODE:
      return CgvhidClient::VkToScancode(VK_MODECHANGE);
    case SDL_SCANCODE_SELECT:
      return CgvhidClient::VkToScancode(VK_SELECT);
    case SDL_SCANCODE_EXECUTE:
      return CgvhidClient::VkToScancode(VK_EXECUTE);
    case SDL_SCANCODE_HELP:
      return CgvhidClient::VkToScancode(VK_HELP);
    case SDL_SCANCODE_PAUSE:
      return CgvhidClient::VkToScancode(VK_PAUSE);
    case SDL_SCANCODE_NUMLOCKCLEAR:
      return CgvhidClient::VkToScancode(VK_NUMLOCK);

    case SDL_SCANCODE_KP_EQUALS:
      return CgvhidClient::VkToScancode(VK_OEM_NEC_EQUAL);
    case SDL_SCANCODE_AC_BACK:
      return CgvhidClient::VkToScancode(VK_BROWSER_BACK);
    case SDL_SCANCODE_AC_FORWARD:
      return CgvhidClient::VkToScancode(VK_BROWSER_FORWARD);
    case SDL_SCANCODE_AC_REFRESH:
      return CgvhidClient::VkToScancode(VK_BROWSER_REFRESH);
    case SDL_SCANCODE_AC_STOP:
      return CgvhidClient::VkToScancode(VK_BROWSER_STOP);
    case SDL_SCANCODE_AC_SEARCH:
      return CgvhidClient::VkToScancode(VK_BROWSER_SEARCH);
    case SDL_SCANCODE_AC_BOOKMARKS:
      return CgvhidClient::VkToScancode(VK_BROWSER_FAVORITES);
    case SDL_SCANCODE_AC_HOME:
      return CgvhidClient::VkToScancode(VK_BROWSER_HOME);
    case SDL_SCANCODE_AUDIOMUTE:
      return CgvhidClient::VkToScancode(VK_VOLUME_MUTE);
    case SDL_SCANCODE_VOLUMEDOWN:
      return CgvhidClient::VkToScancode(VK_VOLUME_DOWN);
    case SDL_SCANCODE_VOLUMEUP:
      return CgvhidClient::VkToScancode(VK_VOLUME_UP);

    case SDL_SCANCODE_AUDIONEXT:
      return CgvhidClient::VkToScancode(VK_MEDIA_NEXT_TRACK);
    case SDL_SCANCODE_AUDIOPREV:
      return CgvhidClient::VkToScancode(VK_MEDIA_PREV_TRACK);
    case SDL_SCANCODE_AUDIOSTOP:
      return CgvhidClient::VkToScancode(VK_MEDIA_STOP);
    case SDL_SCANCODE_AUDIOPLAY:
      return CgvhidClient::VkToScancode(VK_MEDIA_PLAY_PAUSE);
    case SDL_SCANCODE_MAIL:
      return CgvhidClient::VkToScancode(VK_LAUNCH_MAIL);
    case SDL_SCANCODE_MEDIASELECT:
      return CgvhidClient::VkToScancode(VK_LAUNCH_MEDIA_SELECT);

    case SDL_SCANCODE_NONUSBACKSLASH:
      return CgvhidClient::VkToScancode(VK_OEM_102);

    case SDL_SCANCODE_SYSREQ:
      return CgvhidClient::VkToScancode(VK_ATTN);
    case SDL_SCANCODE_CRSEL:
      return CgvhidClient::VkToScancode(VK_CRSEL);
    case SDL_SCANCODE_EXSEL:
      return CgvhidClient::VkToScancode(VK_EXSEL);
    case SDL_SCANCODE_CLEAR:
      return CgvhidClient::VkToScancode(VK_OEM_CLEAR);

    case SDL_SCANCODE_APP1:
      return CgvhidClient::VkToScancode(VK_LAUNCH_APP1);
    case SDL_SCANCODE_APP2:
      return CgvhidClient::VkToScancode(VK_LAUNCH_APP2);

    default:
      if (sdl_scancode & 0xFF00) {
        return KEY_NONE;
      } else {
        return static_cast<uint8_t>(sdl_scancode);
      }
  }
}

// SDL mouse button to CGVHID
CgvhidMouseButton MouseButtonMap(std::uint8_t sdl_button) {
  switch (sdl_button) {
    case SDL_BUTTON_LEFT:
    case SDL_BUTTON_X1:
    case SDL_BUTTON_X2:
      return static_cast<CgvhidMouseButton>(SDL_BUTTON(sdl_button));
    case SDL_BUTTON_MIDDLE:
      return CgvhidMouseButton::kCgvhidMouseButtonMiddle;
    case SDL_BUTTON_RIGHT:
      return CgvhidMouseButton::kCgvhidMouseButtonRight;
    default:
      return kCgvhidMouseButtonNone;
  }
}
}

void GameControl::Cgvhid::ReplayKeyboard(const regame::ClientKeyboard& k) {
  std::uint16_t key_code = ntohs(k.key_code);
  std::uint8_t scan_code = SdlScancodeToHid(key_code);
  if (KEY_NONE == scan_code) {
    APP_WARNING() << "Unknown key code: " << key_code << '\n';
    return;
  }
  if (IsDisableKeys(scan_code)) {
    APP_INFO() << "Disabled scan code: " << static_cast<int>(scan_code) << '\n';
    return;
  }
  if (regame::ButtonState::Pressed == k.state) {
    client_.KeyboardPress(scan_code);
  } else {
    client_.KeyboardRelease(scan_code);
  }
}

void GameControl::Cgvhid::ReplayKeyboardVk(const regame::ClientKeyboard& k) {
  std::uint16_t key_code = ntohs(k.key_code);
  if (key_code & 0xFF00) {
    APP_WARNING() << "Unknown key code: " << key_code;
    return;
  }
  std::uint8_t scan_code =
      CgvhidClient::VkToScancode(static_cast<std::uint8_t>(key_code));
  if (IsDisableKeys(scan_code)) {
    APP_INFO() << "Disabled scan code: " << static_cast<int>(scan_code) << '\n';
    return;
  }
  if (regame::ButtonState::Pressed == k.state) {
    client_.KeyboardPress(scan_code);
  } else {
    client_.KeyboardRelease(scan_code);
  }
}

void GameControl::Cgvhid::ReplayMouseMove(const regame::ClientMouseMove& mm) {
  // Enterprise version
}

void GameControl::Cgvhid::ReplayMouseButton(
    const regame::ClientMouseButton& mb) {
  // Enterprise version
}

void GameControl::Cgvhid::ReplayMouseWheel(const regame::ClientMouseWheel& mw) {
  // Enterprise version
}
