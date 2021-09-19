#include "pch.h"

#include "game_control.h"

#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_scancode.h>

#include <format>

#include "engine.h"
#include "game_service.h"
#include "vigem_client.h"

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

// SDL scancode to Windows scancode
std::uint8_t ScanCodeMap(std::uint16_t key_code) {
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

}  // namespace

void GameControl::Initialize() noexcept {
  gamepad_replay_ = game_service_.GetGamepadReplay();
  keyboard_replay_ = game_service_.GetKeyboardReplay();
  mouse_replay_ = game_service_.GetMouseReplay();

  if (GamepadReplay::kCgvhid == gamepad_replay_) {
    // Not ready
    gamepad_replay_ = GamepadReplay::kNone;
  } else if (GamepadReplay::kVigem == gamepad_replay_) {
    vigem_client_ = std::make_shared<ViGEmClient>();
    if (nullptr != vigem_client_->GetHandle()) {
      vigem_target_x360_ = vigem_client_->CreateController();
    } else {
      gamepad_replay_ = GamepadReplay::kNone;
      APP_ERROR() << "Initialize ViGEmClient failed!\n";
    }
  }

  if (KeyboardReplay::kCgvhid == keyboard_replay_ ||
      MouseReplay::kCgvhid == mouse_replay_) {
    cgvhid_client_.Init(0, 0);
  }

  if (KeyboardReplay::kCgvhid == keyboard_replay_) {
    int error_code = cgvhid_client_.KeyboardReset();
    if (0 != error_code) {
      keyboard_replay_ = KeyboardReplay::kNone;
      APP_ERROR() << "KeyboardReset() failed with " << error_code << '\n';
    }
  }

  if (MouseReplay::kCgvhid == mouse_replay_) {
    int error_code = cgvhid_client_.MouseReset();
    if (0 != error_code) {
      mouse_replay_ = MouseReplay::kNone;
      APP_ERROR() << "MouseReset() failed with " << error_code << '\n';
    }
  }
}

void GameControl::Replay(const regame::ClientControl* cc,
                         std::uint32_t size) noexcept {
  assert(nullptr != cc);
  switch (cc->type) {
    case regame::ControlType::kGamepadAxis:
      if (sizeof(regame::ClientGamepadAxis) == size) {
        ReplayGamepadAxis(*static_cast<const regame::ClientGamepadAxis*>(cc));
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kGamepadButton:
      if (sizeof(regame::ClientGamepadButton) == size) {
        ReplayGamepadButton(
            *static_cast<const regame::ClientGamepadButton*>(cc));
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kKeyboard:
      if (sizeof(regame::ClientKeyboard) == size) {
        ReplayKeyboard(*static_cast<const regame::ClientKeyboard*>(cc));
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kKeyboardVk:
      if (sizeof(regame::ClientKeyboard) == size) {
        ReplayKeyboardVk(*static_cast<const regame::ClientKeyboard*>(cc));
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kMouseMove:
      if (sizeof(regame::ClientMouseMove) == size) {
        ReplayMouseMove(*static_cast<const regame::ClientMouseMove*>(cc));
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kMouseButton:
      if (sizeof(regame::ClientMouseButton) == size) {
        ReplayMouseButton(*static_cast<const regame::ClientMouseButton*>(cc));
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kMouseWheel:
      if (sizeof(regame::ClientMouseWheel) == size) {
        ReplayMouseWheel(*static_cast<const regame::ClientMouseWheel*>(cc));
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    default:
      break;
  }
}

void GameControl::ReplayGamepadAxis(const regame::ClientGamepadAxis& ga) {
  DEBUG_VERBOSE(std::format("{}: which {} axis {}, value {}\n", __func__,
                            static_cast<int>(ga.which),
                            static_cast<int>(ga.axis), ntohs(ga.value)));
  if (GamepadReplay::kVigem == gamepad_replay_) {
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
    vigem_target_x360_->SetState(gamepad_state_);
  }
}

void GameControl::ReplayGamepadButton(const regame::ClientGamepadButton& gb) {
  DEBUG_VERBOSE(std::format("{}: which {} button {}, state {}\n", __func__,
                            static_cast<int>(gb.which),
                            static_cast<int>(gb.button),
                            static_cast<int>(gb.state)));
  if (GamepadReplay::kVigem == gamepad_replay_) {
    if (regame::ButtonState::Pressed == gb.state) {
      gamepad_state_.wButtons |= GamepadButtonMap(gb.button);
    } else {
      gamepad_state_.wButtons &= ~GamepadButtonMap(gb.button);
    }
    vigem_target_x360_->SetState(gamepad_state_);
  }
}

void GameControl::ReplayKeyboard(const regame::ClientKeyboard& k) {
  DEBUG_VERBOSE(std::format("{}: scan code {}, state {}\n", __func__,
                            static_cast<int>(ntohs(k.key_code)),
                            static_cast<int>(k.state)));
  if (KeyboardReplay::kCgvhid == keyboard_replay_) {
    std::uint16_t key_code = ntohs(k.key_code);
    std::uint8_t scan_code = ScanCodeMap(key_code);
    if (KEY_NONE == scan_code) {
      APP_WARNING() << "Unknown key code: " << key_code << '\n';
      return;
    }
    if (game_service_.GetDisableKeys()[scan_code]) {
      APP_INFO() << "Disabled scan code: " << static_cast<int>(scan_code)
                 << '\n';
      return;
    }
    if (regame::ButtonState::Pressed == k.state) {
      cgvhid_client_.KeyboardPress(scan_code);
    } else {
      cgvhid_client_.KeyboardRelease(scan_code);
    }
  }
}

void GameControl::ReplayKeyboardVk(const regame::ClientKeyboard& k) {
  DEBUG_VERBOSE(std::format("{}: scan code {}, state {}\n", __func__,
                            static_cast<int>(ntohs(k.key_code)),
                            static_cast<int>(k.state)));
  if (KeyboardReplay::kCgvhid == keyboard_replay_) {
    std::uint16_t key_code = ntohs(k.key_code);
    if (key_code & 0xFF00) {
      APP_WARNING() << "Unknown key code: " << key_code;
      return;
    }
    std::uint8_t scan_code =
        CgvhidClient::VkToScancode(static_cast<std::uint8_t>(key_code));
    if (game_service_.GetDisableKeys()[scan_code]) {
      APP_INFO() << "Disabled scan code: " << static_cast<int>(scan_code)
                 << '\n';
      return;
    }
    if (regame::ButtonState::Pressed == k.state) {
      cgvhid_client_.KeyboardPress(scan_code);
    } else {
      cgvhid_client_.KeyboardRelease(scan_code);
    }
  }
}

void GameControl::ReplayMouseMove(const regame::ClientMouseMove& mm) {
  DEBUG_VERBOSE(
      std::format("{}: ({}, {})\n", __func__, ntohs(mm.x), ntohs(mm.y)));
  // Enterprise version
}

void GameControl::ReplayMouseButton(const regame::ClientMouseButton& mb) {
  DEBUG_VERBOSE(std::format("{}: ({}, {}), button {}, state {}\n", __func__,
                            ntohs(mb.x), ntohs(mb.y), mb.button,
                            static_cast<int>(mb.state)));
  // Enterprise version
}

void GameControl::ReplayMouseWheel(const regame::ClientMouseWheel& mw) {
  DEBUG_VERBOSE(std::format("{}: ({}, {})\n", __func__, mw.x, mw.y));
  // Enterprise version
}
