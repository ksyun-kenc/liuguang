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

#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_scancode.h>

#include <format>

#include "vigem_client.h"

namespace {

inline void Fail(beast::error_code ec, std::string_view what) {
  APP_ERROR() << "UdpServer " << what << ": "
              << "#" << ec.value() << ", " << ec.message() << '\n';
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
  switch (static_cast<CgvhidGamepadHat>(hat)) {
    case CgvhidGamepadHat::kUp:
      return XINPUT_GAMEPAD_DPAD_UP;
    case CgvhidGamepadHat::kRightUp:
      return XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_RIGHT;
    case CgvhidGamepadHat::kRight:
      return XINPUT_GAMEPAD_DPAD_RIGHT;
    case CgvhidGamepadHat::kRightDown:
      return XINPUT_GAMEPAD_DPAD_RIGHT | XINPUT_GAMEPAD_DPAD_DOWN;
    case CgvhidGamepadHat::kDown:
      return XINPUT_GAMEPAD_DPAD_DOWN;
    case CgvhidGamepadHat::kLeftDown:
      return XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT;
    case CgvhidGamepadHat::kLeft:
      return XINPUT_GAMEPAD_DPAD_LEFT;
    case CgvhidGamepadHat::kLeftUp:
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

// SDL scancode to Windows scancode
uint8_t ScanCodeMap(uint16_t key_code) {
  switch (key_code) {
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
      if (key_code & 0xFF00) {
        return KEY_NONE;
      } else {
        return static_cast<uint8_t>(key_code);
      }
  }
}

// SDL mouse button to CGVHID
CgvhidMouseButton MouseButtonMap(uint8_t sdl_button) {
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

}  // namespace

UdpServer::UdpServer(Engine& engine,
                     udp::endpoint endpoint,
                     std::vector<uint8_t> disable_keys,
                     KeyboardReplay keyboard_replay,
                     GamepadReplay gamepad_replay)
    : engine_(engine),
      socket_(engine.GetIoContext(), endpoint),
      keyboard_replay_(keyboard_replay),
      gamepad_replay_(gamepad_replay) {
  for (const auto& e : disable_keys) {
    disable_keys_[e] = true;
  }

  if (KeyboardReplay::kCgvhid == keyboard_replay) {
    cgvhid_client_.Init(0, 0);
    int error_code = cgvhid_client_.KeyboardReset();
    if (0 != error_code) {
      keyboard_replay = KeyboardReplay::kNone;
      APP_ERROR() << "KeyboardReset() failed with " << error_code << '\n';
    }
  }

  if (GamepadReplay::kCgvhid == gamepad_replay) {
    // Not ready
    gamepad_replay = GamepadReplay::kNone;
  } else if (GamepadReplay::kVigem == gamepad_replay) {
    vigem_client_ = std::make_shared<ViGEmClient>();
    if (nullptr != vigem_client_->GetHandle()) {
      vigem_target_x360_ = vigem_client_->CreateController();
    } else {
      gamepad_replay = GamepadReplay::kNone;
      APP_ERROR() << "Initialize ViGEmClient failed!\n";
    }
  }
}

UdpServer::~UdpServer() {
  cgvhid_client_.KeyboardReset();
}

void UdpServer::OnRead(const boost::system::error_code& ec,
                       std::size_t bytes_transferred) {
  if (ec) {
    if (net::error::operation_aborted == ec) {
      return;
    }
    return Fail(ec, "receive_from");
  }

  if (bytes_transferred >= sizeof(regame::ControlBase)) {
    OnControlEvent(bytes_transferred);
  }
  Read();
}

void UdpServer::OnWrite(const boost::system::error_code& ec,
                        std::size_t bytes_transferred) {}

void UdpServer::OnControlEvent(std::size_t bytes_transferred) noexcept {
  auto control_element =
      reinterpret_cast<regame::ControlElement*>(recv_buffer_.data());
  switch (static_cast<regame::ControlType>(control_element->base.type)) {
    case regame::ControlType::kKeyboard:
      OnKeyboardEvent(bytes_transferred, control_element);
      break;
    case regame::ControlType::kKeyboardVk:
      OnKeyboardVkEvent(bytes_transferred, control_element);
      break;
    case regame::ControlType::kAbsoluteMouseMove:
      break;
    case regame::ControlType::kAbsoluteMouseButton:
      break;
    case regame::ControlType::kAbsoluteMouseWheel:
      break;
    case regame::ControlType::kRelativeMouseMove:
      OnRelativeMouseMoveEvent(bytes_transferred, control_element);
      break;
    case regame::ControlType::kRelativeMouseButton:
      OnRelativeMouseButtonEvent(bytes_transferred, control_element);
      break;
    case regame::ControlType::kRelativeMouseWheel:
      OnRelativeMouseWheelEvent(bytes_transferred, control_element);
      break;
    case regame::ControlType::kGamepadAxis:
      OnGamepadAxisEvent(bytes_transferred, control_element);
      break;
    case regame::ControlType::kGamepadButton:
      OnGamepadButtonEvent(bytes_transferred, control_element);
      break;
    case regame::ControlType::kJoystickAxis:
      // cgc old version use JOYSTICK
      OnJoystickAxisEvent(bytes_transferred, control_element);
      break;
    case regame::ControlType::kJoystickButton:
      // cgc old version use JOYSTICK
      OnJoystickButtonEvent(bytes_transferred, control_element);
      break;
    case regame::ControlType::kJoystickHat:
      // cgc old version use JOYSTICK
      OnJoystickHatEvent(bytes_transferred, control_element);
      break;
  }
}

void UdpServer::OnKeyboardEvent(
    std::size_t bytes_transferred,
    regame::ControlElement* control_element) noexcept {
  assert(nullptr != control_element);

  if (bytes_transferred < sizeof(regame::ControlKeyboard)) {
    return;
  }
  if (KeyboardReplay::kCgvhid == keyboard_replay_) {
    uint16_t key_code = ntohs(control_element->keyboard.key_code);
    uint8_t scan_code = ScanCodeMap(key_code);
    if (KEY_NONE == scan_code) {
      APP_WARNING() << "Unknown key code: " << key_code << '\n';
      return;
    }
    if (disable_keys_[scan_code]) {
      APP_INFO() << "Disabled scan code: " << static_cast<int>(scan_code)
                 << '\n';
      return;
    }
    if (regame::ControlButtonState::Pressed ==
        control_element->keyboard.state) {
      cgvhid_client_.KeyboardPress(scan_code);
    } else {
      cgvhid_client_.KeyboardRelease(scan_code);
    }
  }
}

void UdpServer::OnKeyboardVkEvent(
    std::size_t bytes_transferred,
    regame::ControlElement* control_element) noexcept {
  assert(nullptr != control_element);

  if (bytes_transferred < sizeof(regame::ControlKeyboard)) {
    return;
  }
  if (KeyboardReplay::kCgvhid == keyboard_replay_) {
    uint16_t key_code = ntohs(control_element->keyboard.key_code);
    if (key_code & 0xFF00) {
      APP_WARNING() << "Unknown key code: " << key_code;
      return;
    }
    // TO-DO: disable-keys
    if (regame::ControlButtonState::Pressed ==
        control_element->keyboard.state) {
      cgvhid_client_.KeyboardVkPress(static_cast<uint8_t>(key_code));
    } else {
      cgvhid_client_.KeyboardVkRelease(static_cast<uint8_t>(key_code));
    }
  }
}

void UdpServer::OnJoystickAxisEvent(
    std::size_t bytes_transferred,
    regame::ControlElement* control_element) noexcept {
  if (bytes_transferred < sizeof(regame::ControlJoystickAxis)) {
    return;
  }

  if (GamepadReplay::kVigem == gamepad_replay_) {
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
    regame::ControlElement* control_element) noexcept {
  if (bytes_transferred < sizeof(regame::ControlJoystickButton)) {
    return;
  }

  if (GamepadReplay::kVigem == gamepad_replay_) {
    auto& button = control_element->joystick_button;
    if (regame::ControlButtonState::Pressed == button.state) {
      gamepad_state_.wButtons |= JoystickButtonMap(button.button);
    } else {
      gamepad_state_.wButtons &= ~JoystickButtonMap(button.button);
    }
    vigem_target_x360_->SetState(gamepad_state_);
  }
}

void UdpServer::OnJoystickHatEvent(
    std::size_t bytes_transferred,
    regame::ControlElement* control_element) noexcept {
  if (bytes_transferred < sizeof(regame::ControlJoystickHat)) {
    return;
  }

  if (GamepadReplay::kVigem == gamepad_replay_) {
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

void UdpServer::OnGamepadAxisEvent(
    std::size_t bytes_transferred,
    regame::ControlElement* control_element) noexcept {
  if (bytes_transferred < sizeof(regame::ControlGamepadAxis)) {
    return;
  }

  if (GamepadReplay::kVigem == gamepad_replay_) {
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

void UdpServer::OnGamepadButtonEvent(
    std::size_t bytes_transferred,
    regame::ControlElement* control_element) noexcept {
  if (bytes_transferred < sizeof(regame::ControlGamepadButton)) {
    return;
  }

  if (GamepadReplay::kVigem == gamepad_replay_) {
    auto& button = control_element->gamepad_button;
    if (regame::ControlButtonState::Pressed == button.state) {
      gamepad_state_.wButtons |= GamepadButtonMap(button.button);
    } else {
      gamepad_state_.wButtons &= ~GamepadButtonMap(button.button);
    }
    vigem_target_x360_->SetState(gamepad_state_);
  }
}

void UdpServer::OnRelativeMouseButtonEvent(
    std::size_t bytes_transferred,
    regame::ControlElement* control_element) noexcept {
  assert(nullptr != control_element);
  if (bytes_transferred < sizeof(regame::ControlRelativeMouseButton)) {
    return;
  }
  DEBUG_VERBOSE(std::format(
      "{}: state {}, button {}\n", __func__,
      static_cast<int>(control_element->relative_mouse_button.state),
      control_element->relative_mouse_button.button));
  if (KeyboardReplay::kCgvhid == keyboard_replay_) {
    CgvhidMouseButton button =
        MouseButtonMap(control_element->relative_mouse_button.button);
    if (kCgvhidMouseButtonNone == button) {
      return;
    }
    if (regame::ControlButtonState::Pressed ==
        control_element->relative_mouse_button.state) {
      cgvhid_client_.RelativeMouseButtonPress(button);
    } else {
      cgvhid_client_.RelativeMouseButtonRelease(button);
    }
  }
}

void UdpServer::OnRelativeMouseMoveEvent(
    std::size_t bytes_transferred,
    regame::ControlElement* control_element) noexcept {
  assert(nullptr != control_element);
  if (bytes_transferred < sizeof(regame::ControlRelativeMouseMove)) {
    return;
  }
  DEBUG_VERBOSE(std::format("{}: ({}, {})\n", __func__,
                            control_element->relative_mouse_move.x,
                            control_element->relative_mouse_move.y));
  if (KeyboardReplay::kCgvhid == keyboard_replay_) {
    cgvhid_client_.RelativeMouseMove(control_element->relative_mouse_move.x,
                                     control_element->relative_mouse_move.y);
  }
}

void UdpServer::OnRelativeMouseWheelEvent(
    std::size_t bytes_transferred,
    regame::ControlElement* control_element) noexcept {
  assert(nullptr != control_element);
  if (bytes_transferred < sizeof(regame::ControlRelativeMouseWheel)) {
    return;
  }
  DEBUG_VERBOSE(std::format("{}: ({}, {})\n", __func__,
                            control_element->relative_mouse_wheel.x,
                            control_element->relative_mouse_wheel.y));
  if (KeyboardReplay::kCgvhid == keyboard_replay_) {
    cgvhid_client_.RelativeMouseWheel(control_element->relative_mouse_wheel.x,
                                      control_element->relative_mouse_wheel.y);
  }
}