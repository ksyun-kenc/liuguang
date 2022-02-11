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

namespace {
std::uint16_t GamepadButtonMap(std::uint16_t button) {
  switch (button) {
    case 0:
      return XINPUT_GAMEPAD_A;
    case 1:
      return XINPUT_GAMEPAD_B;
    case 2:
      return XINPUT_GAMEPAD_X;
    case 3:
      return XINPUT_GAMEPAD_Y;
    case 4:
      return XINPUT_GAMEPAD_BACK;
    case 5:
      return XUSB_GAMEPAD_GUIDE;
    case 6:
      return XINPUT_GAMEPAD_START;
    case 7:
      return XINPUT_GAMEPAD_LEFT_THUMB;
    case 8:
      return XINPUT_GAMEPAD_RIGHT_THUMB;
    case 9:
      return XINPUT_GAMEPAD_LEFT_SHOULDER;
    case 10:
      return XINPUT_GAMEPAD_RIGHT_SHOULDER;
    case 11:
      return XINPUT_GAMEPAD_DPAD_UP;
    case 12:
      return XINPUT_GAMEPAD_DPAD_DOWN;
    case 13:
      return XINPUT_GAMEPAD_DPAD_LEFT;
    case 14:
      return XINPUT_GAMEPAD_DPAD_RIGHT;
    default:
      return 0;
  }
}
}  // namespace

void GameControl::ViGEm::ReplayGamepadAxis(
    const regame::ClientGamepadAxis& ga) {
  auto value = ntohs(ga.value);
  switch (ga.axis) {
    case 0:
      gamepad_state_.sThumbLX = value;
      break;
    case 1:
      gamepad_state_.sThumbLY = 0xFFFF - value;
      break;
    case 2:
      gamepad_state_.sThumbRX = value;
      break;
    case 3:
      gamepad_state_.sThumbRY = 0xFFFF - value;
      break;
    case 4:
      gamepad_state_.bLeftTrigger = value >> 7;
      break;
    case 5:
      gamepad_state_.bRightTrigger = value >> 7;
      break;
  }
  target_x360_->SetState(gamepad_state_);
}

void GameControl::ViGEm::ReplayGamepadButton(
    const regame::ClientGamepadButton& gb) {
  if (regame::ButtonState::Pressed == gb.state) {
    gamepad_state_.wButtons |= GamepadButtonMap(gb.button);
  } else {
    gamepad_state_.wButtons &= ~GamepadButtonMap(gb.button);
  }
  target_x360_->SetState(gamepad_state_);
}
