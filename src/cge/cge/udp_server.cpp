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

#include "udp_server.h"

#include "regame/control.h"
#include "vigem_client.h"

namespace {

inline void Fail(beast::error_code ec, std::string_view what) {
  std::cerr << "UdpServer# " << what << ": " << ec.message() << '\n';
}

uint16_t GamepadButtonMap(uint16_t button) {
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
      return XINPUT_GAMEPAD_LEFT_SHOULDER;
    case 5:
      return XINPUT_GAMEPAD_RIGHT_SHOULDER;
    case 6:
      return XINPUT_GAMEPAD_BACK;
    case 7:
      return XINPUT_GAMEPAD_START;
    case 8:
      return XINPUT_GAMEPAD_LEFT_THUMB;
    case 9:
      return XINPUT_GAMEPAD_RIGHT_THUMB;
    case 10:
    default:
      return 0;
  }
}

uint16_t GamepadHatMap(uint16_t button) {
  switch (button) {
    case 1:
      return XINPUT_GAMEPAD_DPAD_UP;
    case 2:
      return XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_RIGHT;
    case 3:
      return XINPUT_GAMEPAD_DPAD_RIGHT;
    case 4:
      return XINPUT_GAMEPAD_DPAD_RIGHT | XINPUT_GAMEPAD_DPAD_DOWN;
    case 5:
      return XINPUT_GAMEPAD_DPAD_DOWN;
    case 6:
      return XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT;
    case 7:
      return XINPUT_GAMEPAD_DPAD_LEFT;
    case 8:
      return XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_UP;
    default:
      return 0;
  }
}

}  // namespace

UdpServer::UdpServer(Engine& engine,
                     udp::endpoint endpoint,
                     KeyboardReplay keyboard_replay,
                     GamepadReplay gamepad_replay)
    : engine_(engine),
      socket_(engine.GetIoContext(), endpoint),
      keyboard_replay_(keyboard_replay),
      gamepad_replay_(gamepad_replay) {
  if (KeyboardReplay::CGVHID == keyboard_replay) {
    cgvhid_client_.Init(0, 0);
    int error_code = cgvhid_client_.KeyboardReset();
    if (0 != error_code) {
      keyboard_replay = KeyboardReplay::NONE;
      std::cerr << "KeyboardReset() failed with " << error_code << '\n';
    }
  }

  if (GamepadReplay::CGVHID == gamepad_replay) {
    // Not ready
    gamepad_replay = GamepadReplay::NONE;
  } else if (GamepadReplay::VIGEM == gamepad_replay) {
    vigem_client_ = std::make_shared<ViGEmClient>();
    if (nullptr != vigem_client_->GetHandle()) {
      vigem_target_x360_ = vigem_client_->CreateController();
    } else {
      gamepad_replay = GamepadReplay::NONE;
    }
  }
}

void UdpServer::OnRead(const boost::system::error_code& ec,
                       std::size_t bytes_transferred) {
  if (ec) {
    if (net::error::operation_aborted == ec) {
      return;
    }
    return Fail(ec, "receive_from");
  }

  if (bytes_transferred >= sizeof(ControlBase)) {
    OnControlEvent(bytes_transferred);
  }
  Read();
}

void UdpServer::OnWrite(const boost::system::error_code& ec,
                        std::size_t bytes_transferred) {}

void UdpServer::OnControlEvent(std::size_t bytes_transferred) noexcept {
  auto control_element = reinterpret_cast<ControlElement*>(recv_buffer_.data());
  switch (static_cast<ControlType>(control_element->base.type)) {
    case ControlType::KEYBOARD:
      if (bytes_transferred < sizeof(ControlKeyboard)) {
        break;
      }
      if (KeyboardReplay::CGVHID == keyboard_replay_) {
        if (kControlKeyboardFlagUp & control_element->keyboard.flags) {
          cgvhid_client_.KeyboardRelease(control_element->keyboard.key_code);
        } else if (kControlKeyboardFlagDown & control_element->keyboard.flags) {
          cgvhid_client_.KeyboardPress(control_element->keyboard.key_code);
        }
      }
      break;
    case ControlType::GAMEPAD:
      if (GamepadReplay::VIGEM == gamepad_replay_) {
        if (kControlGamepadFlagHat & control_element->base.flags) {
          if (bytes_transferred < sizeof(ControlGamepadHat)) {
            break;
          }
          auto hat =
              reinterpret_cast<const ControlGamepadHat*>(control_element);
          gamepad_state_.wButtons &=
              ~(XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN |
                XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT);
          if (hat->hat) {
            gamepad_state_.wButtons |= GamepadHatMap(hat->hat);
          }
          vigem_target_x360_->SetState(gamepad_state_);
        }

        if (kControlGamepadFlagAxis & control_element->base.flags) {
          if (bytes_transferred < sizeof(ControlGamepadAxis)) {
            break;
          }
          auto axis =
              reinterpret_cast<const ControlGamepadAxis*>(control_element);
          switch (axis->axis_index) {
            case 0:
              gamepad_state_.sThumbLX = axis->coordinates - 0x8000;
              break;
            case 1:
              gamepad_state_.sThumbLY = 0x7fff - axis->coordinates;
              break;
            case 2:
              gamepad_state_.bLeftTrigger = axis->coordinates >> 2;
              break;
            case 3:
              gamepad_state_.sThumbRX = axis->coordinates - 0x8000;
              break;
            case 4:
              gamepad_state_.sThumbRY = 0x7fff - axis->coordinates;
              break;
            case 5:
              gamepad_state_.bRightTrigger = axis->coordinates >> 2;
              break;
          }
          vigem_target_x360_->SetState(gamepad_state_);
        }

        if (kControlGamepadFlagButton & control_element->base.flags) {
          if (bytes_transferred < sizeof(ControlGamepadButton)) {
            break;
          }
          auto button =
              reinterpret_cast<const ControlGamepadButton*>(control_element);
          if (kControlGamepadFlagDown & control_element->base.flags) {
            gamepad_state_.wButtons |= GamepadButtonMap(button->button);
          } else if (kControlGamepadFlagUp & control_element->base.flags) {
            gamepad_state_.wButtons &= ~GamepadButtonMap(button->button);
          } else {
            break;
          }
          vigem_target_x360_->SetState(gamepad_state_);
        }
      }

      break;
  }
}