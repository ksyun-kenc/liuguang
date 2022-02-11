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

#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_scancode.h>

inline std::uint8_t SdlScancodeToVk(std::uint16_t sdl_scancode) {
  switch (sdl_scancode) {
    case SDL_SCANCODE_MODE:
      return VK_MODECHANGE;
    case SDL_SCANCODE_SELECT:
      return VK_SELECT;
    case SDL_SCANCODE_EXECUTE:
      return VK_EXECUTE;
    case SDL_SCANCODE_HELP:
      return VK_HELP;
    case SDL_SCANCODE_PAUSE:
      return VK_PAUSE;
    case SDL_SCANCODE_NUMLOCKCLEAR:
      return VK_NUMLOCK;

    case SDL_SCANCODE_F13:
      return VK_F13;
    case SDL_SCANCODE_F14:
      return VK_F14;
    case SDL_SCANCODE_F15:
      return VK_F15;
    case SDL_SCANCODE_F16:
      return VK_F16;
    case SDL_SCANCODE_F17:
      return VK_F17;
    case SDL_SCANCODE_F18:
      return VK_F18;
    case SDL_SCANCODE_F19:
      return VK_F19;
    case SDL_SCANCODE_F20:
      return VK_F20;
    case SDL_SCANCODE_F21:
      return VK_F21;
    case SDL_SCANCODE_F22:
      return VK_F22;
    case SDL_SCANCODE_F23:
      return VK_F23;
    case SDL_SCANCODE_F24:
      return VK_F24;

    case SDL_SCANCODE_KP_EQUALS:
      return VK_OEM_NEC_EQUAL;
    case SDL_SCANCODE_AC_BACK:
      return VK_BROWSER_BACK;
    case SDL_SCANCODE_AC_FORWARD:
      return VK_BROWSER_FORWARD;
    case SDL_SCANCODE_AC_REFRESH:
      return VK_BROWSER_REFRESH;
    case SDL_SCANCODE_AC_STOP:
      return VK_BROWSER_STOP;
    case SDL_SCANCODE_AC_SEARCH:
      return VK_BROWSER_SEARCH;
    case SDL_SCANCODE_AC_BOOKMARKS:
      return VK_BROWSER_FAVORITES;
    case SDL_SCANCODE_AC_HOME:
      return VK_BROWSER_HOME;
    case SDL_SCANCODE_AUDIOMUTE:
      return VK_VOLUME_MUTE;
    case SDL_SCANCODE_VOLUMEDOWN:
      return VK_VOLUME_DOWN;
    case SDL_SCANCODE_VOLUMEUP:
      return VK_VOLUME_UP;

    case SDL_SCANCODE_AUDIONEXT:
      return VK_MEDIA_NEXT_TRACK;
    case SDL_SCANCODE_AUDIOPREV:
      return VK_MEDIA_PREV_TRACK;
    case SDL_SCANCODE_AUDIOSTOP:
      return VK_MEDIA_STOP;
    case SDL_SCANCODE_AUDIOPLAY:
      return VK_MEDIA_PLAY_PAUSE;
    case SDL_SCANCODE_MAIL:
      return VK_LAUNCH_MAIL;
    case SDL_SCANCODE_MEDIASELECT:
      return VK_LAUNCH_MEDIA_SELECT;

    case SDL_SCANCODE_NONUSBACKSLASH:
      return VK_OEM_102;

    case SDL_SCANCODE_SYSREQ:
      return VK_ATTN;
    case SDL_SCANCODE_CRSEL:
      return VK_CRSEL;
    case SDL_SCANCODE_EXSEL:
      return VK_EXSEL;
    case SDL_SCANCODE_CLEAR:
      return VK_OEM_CLEAR;

    case SDL_SCANCODE_APP1:
      return VK_LAUNCH_APP1;
    case SDL_SCANCODE_APP2:
      return VK_LAUNCH_APP2;

    default:
      if (sdl_scancode & 0xFF00) {
        return KEY_NONE;
      } else {
        return static_cast<uint8_t>(sdl_scancode);
      }
  }
}
