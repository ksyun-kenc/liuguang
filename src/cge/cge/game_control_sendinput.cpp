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
#include "windows_scancode.h"

namespace {

WindowsScancode SdlScancodeToWindows(SDL_Scancode sdl_scancode) {
  using enum WindowsScancode;

  switch (sdl_scancode) {
    case SDL_SCANCODE_ESCAPE:
      return kEscape;
    case SDL_SCANCODE_1:
      return k1;
    case SDL_SCANCODE_2:
      return k2;
    case SDL_SCANCODE_3:
      return k3;
    case SDL_SCANCODE_4:
      return k4;
    case SDL_SCANCODE_5:
      return k5;
    case SDL_SCANCODE_6:
      return k6;
    case SDL_SCANCODE_7:
      return k7;
    case SDL_SCANCODE_8:
      return k8;
    case SDL_SCANCODE_9:
      return k9;
    case SDL_SCANCODE_0:
      return k0;
    case SDL_SCANCODE_MINUS:
      return kMinus;
    case SDL_SCANCODE_EQUALS:
      return kEquals;
    case SDL_SCANCODE_BACKSPACE:
      return kBackspace;
    case SDL_SCANCODE_TAB:
      return kTab;
    case SDL_SCANCODE_Q:
      return kQ;
    case SDL_SCANCODE_W:
      return kW;
    case SDL_SCANCODE_E:
      return kE;
    case SDL_SCANCODE_R:
      return kR;
    case SDL_SCANCODE_T:
      return kT;
    case SDL_SCANCODE_Y:
      return kY;
    case SDL_SCANCODE_U:
      return kU;
    case SDL_SCANCODE_I:
      return kI;
    case SDL_SCANCODE_O:
      return kO;
    case SDL_SCANCODE_P:
      return kP;
    case SDL_SCANCODE_LEFTBRACKET:
      return kLeftBracket;
    case SDL_SCANCODE_RIGHTBRACKET:
      return kRightBracket;
    case SDL_SCANCODE_RETURN:
      return kReturn;
    case SDL_SCANCODE_LCTRL:
      return kLeftControl;
    case SDL_SCANCODE_A:
      return kA;
    case SDL_SCANCODE_S:
      return kS;
    case SDL_SCANCODE_D:
      return kD;
    case SDL_SCANCODE_F:
      return kF;
    case SDL_SCANCODE_G:
      return kG;
    case SDL_SCANCODE_H:
      return kH;
    case SDL_SCANCODE_J:
      return kJ;
    case SDL_SCANCODE_K:
      return kK;
    case SDL_SCANCODE_L:
      return kL;
    case SDL_SCANCODE_SEMICOLON:
      return kSemicolon;
    case SDL_SCANCODE_APOSTROPHE:
      return kApostrophe;
    case SDL_SCANCODE_GRAVE:
      return kGrave;
    case SDL_SCANCODE_LSHIFT:
      return kLeftShift;
    case SDL_SCANCODE_BACKSLASH:
      return kBackslash;
    case SDL_SCANCODE_Z:
      return kZ;
    case SDL_SCANCODE_X:
      return kX;
    case SDL_SCANCODE_C:
      return kC;
    case SDL_SCANCODE_V:
      return kV;
    case SDL_SCANCODE_B:
      return kB;
    case SDL_SCANCODE_N:
      return kN;
    case SDL_SCANCODE_M:
      return kM;
    case SDL_SCANCODE_COMMA:
      return kComma;
    case SDL_SCANCODE_PERIOD:
      return kPreiod;
    case SDL_SCANCODE_SLASH:
      return kSlash;
    case SDL_SCANCODE_RSHIFT:
      return kRightShift;
    case SDL_SCANCODE_KP_MULTIPLY:
      return kNumpadMultiply;
    case SDL_SCANCODE_LALT:
      return kLeftAlt;
    case SDL_SCANCODE_SPACE:
      return kSpace;
    case SDL_SCANCODE_CAPSLOCK:
      return kCapsLock;
    case SDL_SCANCODE_F1:
      return kF1;
    case SDL_SCANCODE_F2:
      return kF2;
    case SDL_SCANCODE_F3:
      return kF3;
    case SDL_SCANCODE_F4:
      return kF4;
    case SDL_SCANCODE_F5:
      return kF5;
    case SDL_SCANCODE_F6:
      return kF6;
    case SDL_SCANCODE_F7:
      return kF7;
    case SDL_SCANCODE_F8:
      return kF8;
    case SDL_SCANCODE_F9:
      return kF9;
    case SDL_SCANCODE_F10:
      return kF10;
    case SDL_SCANCODE_NUMLOCKCLEAR:
      return kNumLock;
    case SDL_SCANCODE_SCROLLLOCK:
      return kScrollLock;
    case SDL_SCANCODE_HOME:  // ?
    case SDL_SCANCODE_KP_7:
      return kNumpad7;
    case SDL_SCANCODE_UP:  // ?
      return kUp;
    case SDL_SCANCODE_KP_8:
      return kNumpad8;
    case SDL_SCANCODE_PAGEUP:  // ?
      return kPageUp;
    case SDL_SCANCODE_KP_9:
      return kNumpad9;
    case SDL_SCANCODE_KP_MINUS:
      return kNumpadMinus;
    case SDL_SCANCODE_LEFT:  // ?123
      return kLeft;
    case SDL_SCANCODE_KP_4:
      return kNumpad4;
    case SDL_SCANCODE_KP_5:
      return kNumpad5;
    case SDL_SCANCODE_RIGHT:  // ?
      return kRight;
    case SDL_SCANCODE_KP_6:
      return kNumpad6;
    case SDL_SCANCODE_KP_PLUS:
      return kNumpadPlus;
    case SDL_SCANCODE_END:  // ?
    case SDL_SCANCODE_KP_1:
      return kNumpad1;
    case SDL_SCANCODE_DOWN:  // ?
    case SDL_SCANCODE_KP_2:
      return kNumpad2;
    case SDL_SCANCODE_PAGEDOWN:  // ?
    case SDL_SCANCODE_KP_3:
      return kNumpad3;
    case SDL_SCANCODE_INSERT:  // ?
    case SDL_SCANCODE_KP_0:
      return kNumpad0;
    case SDL_SCANCODE_DELETE:  // ?
    case SDL_SCANCODE_KP_PERIOD:
      return kNumpadPeriod;
    case SDL_SCANCODE_PRINTSCREEN:
      return kAltPrintScreen;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 85;
    case SDL_SCANCODE_NONUSBACKSLASH:
      return kBracketAngle;
    case SDL_SCANCODE_F11:
      return kF11;
    case SDL_SCANCODE_F12:
      return kF12;
    // case SDL_SCANCODE_PAUSE:
    //  return 89;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 90;
    case SDL_SCANCODE_LGUI:
      return kOemFinish;
    case SDL_SCANCODE_RGUI:
      return kOemJump;
    case SDL_SCANCODE_APPLICATION:
      return kEraseEOF;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 94;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 95;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 96;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 97;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 98;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 99;
    case SDL_SCANCODE_F13:
      return kF13;
    case SDL_SCANCODE_F14:
      return kF14;
    case SDL_SCANCODE_F15:
      return kF15;
    case SDL_SCANCODE_F16:
      return kF16;
    case SDL_SCANCODE_F17:
      return kF17;
    case SDL_SCANCODE_F18:
      return kF18;
    case SDL_SCANCODE_F19:
      return kF19;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 107;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 108;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 109;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 110;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 111;
    case SDL_SCANCODE_INTERNATIONAL2:
      return kKatakana;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 113;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 114;
    case SDL_SCANCODE_INTERNATIONAL1:
      return kReserved1;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 116;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 117;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 118;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 119;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 120;
    case SDL_SCANCODE_INTERNATIONAL4:
      return kConvert;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 122;
    case SDL_SCANCODE_INTERNATIONAL5:
      return kNonConvert;
    // case SDL_SCANCODE_UNKNOWN:
    //   return 124;
    case SDL_SCANCODE_INTERNATIONAL3:
      return kUnkown1;
      // case SDL_SCANCODE_UNKNOWN:
      //   return kReserved2;
      // case SDL_SCANCODE_UNKNOWN:
      //   return 127;
    default:
      return kNone;
  }
}

WORD SdlMouseButtonToWindows(std::uint8_t sdl_button,
                             regame::ButtonState state) {
  switch (sdl_button) {
    case SDL_BUTTON_LEFT:
      if (regame::ButtonState::Pressed == state) {
        return MOUSEEVENTF_LEFTDOWN;
      }
      return MOUSEEVENTF_LEFTUP;
    case SDL_BUTTON_X1:
    case SDL_BUTTON_X2:
      if (regame::ButtonState::Pressed == state) {
        return MOUSEEVENTF_XDOWN;
      }
      return MOUSEEVENTF_XUP;
    case SDL_BUTTON_MIDDLE:
      if (regame::ButtonState::Pressed == state) {
        return MOUSEEVENTF_MIDDLEDOWN;
      }
      return MOUSEEVENTF_MIDDLEUP;
    case SDL_BUTTON_RIGHT:
      if (regame::ButtonState::Pressed == state) {
        return MOUSEEVENTF_RIGHTDOWN;
      }
      return MOUSEEVENTF_RIGHTUP;
    default:
      return 0;
  }
}
}  // namespace

void GameControl::SendInput::ReplayKeyboard(const regame::ClientKeyboard& k) {
  std::uint16_t key_code = ntohs(k.key_code);
  WindowsScancode sc =
      SdlScancodeToWindows(static_cast<SDL_Scancode>(key_code));
  if (WindowsScancode::kNone == sc) {
    APP_WARNING() << "Unknown key code: " << static_cast<int>(sc) << '\n';
    return;
  }
  // TO-DO
  UINT vk = SdlScancodeToVk(key_code);
  if (IsDisableKeys(vk)) {
    APP_INFO() << "Disabled scan code: " << static_cast<int>(vk) << '\n';
    return;
  }
  INPUT input{};
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = 0;
  input.ki.wScan = static_cast<std::uint16_t>(sc);
  input.ki.dwFlags = KEYEVENTF_SCANCODE;
  if (regame::ButtonState::Released == k.state) {
    input.ki.dwFlags |= KEYEVENTF_KEYUP;
  }
  ::SendInput(1, &input, sizeof(input));
}

void GameControl::SendInput::ReplayKeyboardVk(const regame::ClientKeyboard& k) {
  std::uint16_t key_code = ntohs(k.key_code);
  if (key_code & 0xFF00) {
    APP_WARNING() << "Unknown key code: " << key_code;
    return;
  }
  std::uint8_t vk = static_cast<std::uint8_t>(key_code);
  if (IsDisableKeys(vk)) {
    APP_INFO() << "Disabled VK: " << static_cast<int>(vk) << '\n';
    return;
  }
  UINT scan_code = MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC_EX, nullptr);
  if (0 == scan_code) {
    APP_WARNING() << "VK to Scancode failed: " << static_cast<int>(vk);
    return;
  }
  INPUT input{};
  input.type = INPUT_KEYBOARD;
  // input.ki.wVk = vk;
  input.ki.wScan = scan_code;
  input.ki.dwFlags = KEYEVENTF_SCANCODE;
  if (regame::ButtonState::Released == k.state) {
    input.ki.dwFlags |= KEYEVENTF_KEYUP;
  }
  ::SendInput(1, &input, sizeof(input));
}

void GameControl::SendInput::ReplayMouseMove(
    const regame::ClientMouseMove& mm) {
  // Enterprise version
}

void GameControl::SendInput::ReplayMouseButton(
    const regame::ClientMouseButton& mb) {
  // Enterprise version
}

void GameControl::SendInput::ReplayMouseWheel(
    const regame::ClientMouseWheel& mw) {
  if (0 != mw.x || 0 != mw.y) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    // input.mi.dx = mouse_last_pos_.x;
    // input.mi.dy = mouse_last_pos_.y;
    if (0 != mw.x) {
      input.mi.mouseData = 120 * mw.x;
      input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
    } else if (0 != mw.y) {
      input.mi.mouseData = 120 * mw.y;
      input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    }
    ::SendInput(1, &input, sizeof(INPUT));
  }
}
