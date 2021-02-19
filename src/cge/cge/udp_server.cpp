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

#include "vigem_client.h"

namespace {

inline void Fail(beast::error_code ec, std::string_view what) {
  std::cerr << "UdpServer# " << what << ": " << ec.message() << '\n';
}

uint16_t JoystickButtonMap(uint16_t button) {
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

uint16_t JoystickHatMap(uint8_t hat) {
  switch (hat) {
    case VhidGamepadHat::UP:
      return XINPUT_GAMEPAD_DPAD_UP;
    case VhidGamepadHat::RIGHTUP:
      return XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_RIGHT;
    case VhidGamepadHat::RIGHT:
      return XINPUT_GAMEPAD_DPAD_RIGHT;
    case VhidGamepadHat::RIGHTDOWN:
      return XINPUT_GAMEPAD_DPAD_RIGHT | XINPUT_GAMEPAD_DPAD_DOWN;
    case VhidGamepadHat::DOWN:
      return XINPUT_GAMEPAD_DPAD_DOWN;
    case VhidGamepadHat::LEFTDOWN:
      return XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT;
    case VhidGamepadHat::LEFT:
      return XINPUT_GAMEPAD_DPAD_LEFT;
    case VhidGamepadHat::LEFTUP:
      return XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_UP;
    default:
      return 0;
  }
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
      std::cerr << "Initialize ViGEmClient failed!\n";
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
      OnKeyboardEvent(bytes_transferred, control_element);
      break;
    case ControlType::KEYBOARD_VK:
      OnKeyboardVkEvent(bytes_transferred, control_element);
      break;
    case ControlType::GAMEPAD_AXIS:
      OnGamepadAxisEvent(bytes_transferred, control_element);
      break;
    case ControlType::GAMEPAD_BUTTON:
      OnGamepadButtonEvent(bytes_transferred, control_element);
      break;
    case ControlType::JOYSTICK_AXIS:
      // cgc old version use JOYSTICK
      OnJoystickAxisEvent(bytes_transferred, control_element);
      break;
    case ControlType::JOYSTICK_BUTTON:
      // cgc old version use JOYSTICK
      OnJoystickButtonEvent(bytes_transferred, control_element);
      break;
    case ControlType::JOYSTICK_HAT:
      // cgc old version use JOYSTICK
      OnJoystickHatEvent(bytes_transferred, control_element);
      break;
  }
}

void UdpServer::OnKeyboardEvent(std::size_t bytes_transferred,
                                ControlElement* control_element) noexcept {
  assert(nullptr != control_element);

  if (bytes_transferred < sizeof(ControlKeyboard)) {
    return;
  }
  if (KeyboardReplay::CGVHID == keyboard_replay_) {
    uint16_t key_code = ntohs(control_element->keyboard.key_code);
    if (key_code & 0xFF00) {
      std::cout << "Unknown key code: " << key_code;
      return;
    }
    if (ControlButtonState::Pressed == control_element->keyboard.state) {
      cgvhid_client_.KeyboardPress(static_cast<uint8_t>(key_code));
    } else {
      cgvhid_client_.KeyboardRelease(static_cast<uint8_t>(key_code));
    }
  }
}

void UdpServer::OnKeyboardVkEvent(std::size_t bytes_transferred,
                                  ControlElement* control_element) noexcept {
  assert(nullptr != control_element);

  if (bytes_transferred < sizeof(ControlKeyboard)) {
    return;
  }
  if (KeyboardReplay::CGVHID == keyboard_replay_) {
    uint16_t key_code = ntohs(control_element->keyboard.key_code);
    if (key_code & 0xFF00) {
      std::cout << "Unknown key code: " << key_code;
      return;
    }
    if (ControlButtonState::Pressed == control_element->keyboard.state) {
      cgvhid_client_.KeyboardVkPress(static_cast<uint8_t>(key_code));
    } else {
      cgvhid_client_.KeyboardVkRelease(static_cast<uint8_t>(key_code));
    }
  }
}

void UdpServer::OnJoystickAxisEvent(std::size_t bytes_transferred,
                                    ControlElement* control_element) noexcept {
  if (bytes_transferred < sizeof(ControlJoystickAxis)) {
    return;
  }

  if (GamepadReplay::VIGEM == gamepad_replay_) {
    auto& axis = control_element->joystick_axis;
    switch (axis.axis) {
      case 0:
        gamepad_state_.sThumbLX = axis.value - 0x8000;
        break;
      case 1:
        gamepad_state_.sThumbLY = 0x7fff - axis.value;
        break;
      case 2:
        gamepad_state_.bLeftTrigger = axis.value >> 2;
        break;
      case 3:
        gamepad_state_.sThumbRX = axis.value - 0x8000;
        break;
      case 4:
        gamepad_state_.sThumbRY = 0x7fff - axis.value;
        break;
      case 5:
        gamepad_state_.bRightTrigger = axis.value >> 2;
        break;
    }
    vigem_target_x360_->SetState(gamepad_state_);
  }
}

void UdpServer::OnJoystickButtonEvent(
    std::size_t bytes_transferred,
    ControlElement* control_element) noexcept {
  if (bytes_transferred < sizeof(ControlJoystickButton)) {
    return;
  }

  if (GamepadReplay::VIGEM == gamepad_replay_) {
    auto& button = control_element->joystick_button;
    if (ControlButtonState::Pressed == button.state) {
      gamepad_state_.wButtons |= JoystickButtonMap(button.button);
    } else {
      gamepad_state_.wButtons &= ~JoystickButtonMap(button.button);
    }
    vigem_target_x360_->SetState(gamepad_state_);
  }
}

void UdpServer::OnJoystickHatEvent(std::size_t bytes_transferred,
                                   ControlElement* control_element) noexcept {
  if (bytes_transferred < sizeof(ControlJoystickHat)) {
    return;
  }

  if (GamepadReplay::VIGEM == gamepad_replay_) {
    auto& hat = control_element->joystick_hat;
    gamepad_state_.wButtons &=
        ~(XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_DOWN |
          XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT);
    if (hat.hat) {
      gamepad_state_.wButtons |= JoystickHatMap(hat.hat);
    }
    vigem_target_x360_->SetState(gamepad_state_);
  }
}

void UdpServer::OnGamepadAxisEvent(std::size_t bytes_transferred,
                                   ControlElement* control_element) noexcept {
  if (bytes_transferred < sizeof(ControlGamepadAxis)) {
    return;
  }

  if (GamepadReplay::VIGEM == gamepad_replay_) {
    auto& axis = control_element->gamepad_axis;
    auto value = ntohs(axis.value);
    switch (axis.axis) {
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
    vigem_target_x360_->SetState(gamepad_state_);
  }
}

void UdpServer::OnGamepadButtonEvent(std::size_t bytes_transferred,
                                     ControlElement* control_element) noexcept {
  if (bytes_transferred < sizeof(ControlGamepadButton)) {
    return;
  }

  if (GamepadReplay::VIGEM == gamepad_replay_) {
    auto& button = control_element->gamepad_button;
    if (ControlButtonState::Pressed == button.state) {
      gamepad_state_.wButtons |= GamepadButtonMap(button.button);
    } else {
      gamepad_state_.wButtons &= ~GamepadButtonMap(button.button);
    }
    vigem_target_x360_->SetState(gamepad_state_);
  }
}
