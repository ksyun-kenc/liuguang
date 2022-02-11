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

UINT GameControl::Message::SdlMouseButtonToMessage(std::uint8_t sdl_button,
                                                   regame::ButtonState state) {
  switch (sdl_button) {
    case SDL_BUTTON_LEFT:
      if (regame::ButtonState::Pressed == state) {
        button_state_ |= MK_LBUTTON;
        return WM_LBUTTONDOWN;
      }
      button_state_ &= ~MK_LBUTTON;
      return WM_LBUTTONUP;
    case SDL_BUTTON_X1:
      if (regame::ButtonState::Pressed == state) {
        button_state_ |= MK_XBUTTON1;
        return WM_XBUTTONDOWN;
      }
      button_state_ &= ~MK_XBUTTON1;
      return WM_XBUTTONUP;
    case SDL_BUTTON_X2:
      if (regame::ButtonState::Pressed == state) {
        button_state_ |= MK_XBUTTON2;
        return WM_XBUTTONDOWN;
      }
      button_state_ &= ~MK_XBUTTON2;
      return WM_XBUTTONUP;
    case SDL_BUTTON_MIDDLE:
      if (regame::ButtonState::Pressed == state) {
        button_state_ |= MK_MBUTTON;
        return WM_MBUTTONDOWN;
      }
      button_state_ &= ~MK_MBUTTON;
      return WM_MBUTTONUP;
    case SDL_BUTTON_RIGHT:
      if (regame::ButtonState::Pressed == state) {
        button_state_ |= MK_RBUTTON;
        return WM_RBUTTONDOWN;
      }
      button_state_ &= ~MK_RBUTTON;
      return WM_RBUTTONUP;
    default:
      return 0;
  }
}

void GameControl::Message::ReplayKeyboard(const regame::ClientKeyboard& k) {}

void GameControl::Message::ReplayKeyboardVk(const regame::ClientKeyboard& k) {}

void GameControl::Message::ReplayMouseMove(const regame::ClientMouseMove& mm) {
  // Enterprise version
}

void GameControl::Message::ReplayMouseButton(
    const regame::ClientMouseButton& mb) {
  // Enterprise version
}

void GameControl::Message::ReplayMouseWheel(
    const regame::ClientMouseWheel& mw) {
  // Enterprise version
}
