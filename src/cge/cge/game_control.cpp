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

#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_scancode.h>

#include <format>

#include "app.hpp"
#include "game_service.h"

std::array<bool, 256> GameControl::disable_keys_{};
POINT GameControl::mouse_last_pos_{};

void GameControl::Initialize() noexcept {
  gamepad_replay_ = game_service_.GetGamepadReplay();
  keyboard_replay_ = game_service_.GetKeyboardReplay();
  mouse_replay_ = game_service_.GetMouseReplay();

  if (GamepadReplay::kCgvhid == gamepad_replay_) {
    // Not ready
    gamepad_replay_ = GamepadReplay::kNone;
  } else if (GamepadReplay::kVigem == gamepad_replay_) {
    vigem_.client_ = std::make_shared<ViGEmClient>();
    if (nullptr != vigem_.client_->GetHandle()) {
      vigem_.target_x360_ = vigem_.client_->CreateController();
    } else {
      gamepad_replay_ = GamepadReplay::kNone;
      APP_ERROR() << "Initialize ViGEmClient failed!\n";
    }
  }

  if (KeyboardReplay::kCgvhid == keyboard_replay_ ||
      MouseReplay::kCgvhid == mouse_replay_) {
    cgvhid_.client_.Initialize(0, 0);
  }

  if (KeyboardReplay::kCgvhid == keyboard_replay_) {
    int error_code = cgvhid_.client_.KeyboardReset();
    if (0 != error_code) {
      keyboard_replay_ = KeyboardReplay::kNone;
      APP_ERROR() << "KeyboardReset() failed with " << error_code << '\n';
    }
  }

  if (MouseReplay::kCgvhid == mouse_replay_) {
    int error_code = cgvhid_.client_.MouseReset();
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
        const auto& ga =
            *reinterpret_cast<const regame::ClientGamepadAxis*>(cc);
        DEBUG_VERBOSE(std::format("GamepadAxis: which {} axis {}, value {}\n",
                                  static_cast<int>(ga.which),
                                  static_cast<int>(ga.axis), ntohs(ga.value)));
        switch (gamepad_replay_) {
          case GamepadReplay::kVigem:
            vigem_.ReplayGamepadAxis(ga);
            break;
        }
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kGamepadButton:
      if (sizeof(regame::ClientGamepadButton) == size) {
        const auto& gb =
            *reinterpret_cast<const regame::ClientGamepadButton*>(cc);
        DEBUG_VERBOSE(
            std::format("GamepadButton: which {} button {}, state {}\n",
                        static_cast<int>(gb.which), static_cast<int>(gb.button),
                        static_cast<int>(gb.state)));
        switch (gamepad_replay_) {
          case GamepadReplay::kVigem:
            vigem_.ReplayGamepadButton(gb);
            break;
        }
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kKeyboard:
      if (sizeof(regame::ClientKeyboard) == size) {
        const auto& k = *reinterpret_cast<const regame::ClientKeyboard*>(cc);
        DEBUG_VERBOSE(std::format("Keyboard: scan code {}, state {}\n",
                                  static_cast<int>(ntohs(k.key_code)),
                                  static_cast<int>(k.state)));
        switch (keyboard_replay_) {
          case KeyboardReplay::kCgvhid:
            cgvhid_.ReplayKeyboard(k);
            break;
          case KeyboardReplay::kSendInput:
            send_input_.ReplayKeyboard(k);
            break;
        }
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kKeyboardVk:
      if (sizeof(regame::ClientKeyboard) == size) {
        const auto& k = *reinterpret_cast<const regame::ClientKeyboard*>(cc);
        DEBUG_VERBOSE(std::format("KeyboardVk: scan code {}, state {}\n",
                                  static_cast<int>(ntohs(k.key_code)),
                                  static_cast<int>(k.state)));
        switch (keyboard_replay_) {
          case KeyboardReplay::kCgvhid:
            cgvhid_.ReplayKeyboardVk(k);
            break;
          case KeyboardReplay::kSendInput:
            send_input_.ReplayKeyboardVk(k);
            break;
        }
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kMouseMove:
      if (sizeof(regame::ClientMouseMove) == size) {
        const auto& mm = *reinterpret_cast<const regame::ClientMouseMove*>(cc);
        DEBUG_VERBOSE(
            std::format("MouseMove: ({}, {})\n", ntohs(mm.x), ntohs(mm.y)));
        switch (mouse_replay_) {
          case MouseReplay::kCgvhid:
            cgvhid_.ReplayMouseMove(mm);
            break;
          case MouseReplay::kSendInput:
            send_input_.ReplayMouseMove(mm);
            break;
          case MouseReplay::kMessage:
            message_.ReplayMouseMove(mm);
            break;
        }
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kRelativeMouseMove:
      if (sizeof(regame::ClientRelativeMouseMove) == size) {
        const auto& rmm =
            *reinterpret_cast<const regame::ClientRelativeMouseMove*>(cc);
        DEBUG_VERBOSE(std::format("RelativeMouseMove: ({}, {})\n", ntohs(rmm.x),
                                  ntohs(rmm.y)));
        switch (mouse_replay_) {
          case MouseReplay::kCgvhid:
            cgvhid_.ReplayRelativeMouseMove(rmm);
            break;
          case MouseReplay::kSendInput:
            send_input_.ReplayRelativeMouseMove(rmm);
            break;
        }
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kMouseButton:
      if (sizeof(regame::ClientMouseButton) == size) {
        const auto& mb =
            *reinterpret_cast<const regame::ClientMouseButton*>(cc);
        DEBUG_VERBOSE(std::format(
            "MouseButton: ({}, {}), button {}, state {}\n", ntohs(mb.x),
            ntohs(mb.y), mb.button, static_cast<int>(mb.state)));
        switch (mouse_replay_) {
          case MouseReplay::kCgvhid:
            cgvhid_.ReplayMouseButton(mb);
            break;
          case MouseReplay::kSendInput:
            send_input_.ReplayMouseButton(mb);
            break;
          case MouseReplay::kMessage:
            message_.ReplayMouseButton(mb);
            break;
        }
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kRelativeMouseButton:
      if (sizeof(regame::ClientRelativeMouseButton) == size) {
        const auto& rmb =
            *reinterpret_cast<const regame::ClientRelativeMouseButton*>(cc);
        DEBUG_VERBOSE(std::format("RelativeMouseButton: button {}, state {}\n",
                                  rmb.button, static_cast<int>(rmb.state)));
        switch (mouse_replay_) {
          case MouseReplay::kCgvhid:
            cgvhid_.ReplayRelativeMouseButton(rmb);
            break;
          case MouseReplay::kSendInput:
            send_input_.ReplayRelativeMouseButton(rmb);
            break;
        }
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kMouseWheel:
      if (sizeof(regame::ClientMouseWheel) == size) {
        const auto& mw = *reinterpret_cast<const regame::ClientMouseWheel*>(cc);
        DEBUG_VERBOSE(std::format("MouseWheel: ({}, {})\n", mw.x, mw.y));
        switch (mouse_replay_) {
          case MouseReplay::kCgvhid:
            cgvhid_.ReplayMouseWheel(mw);
            break;
          case MouseReplay::kSendInput:
            send_input_.ReplayMouseWheel(mw);
            break;
          case MouseReplay::kMessage:
            message_.ReplayMouseWheel(mw);
            break;
        }
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    case regame::ControlType::kRelativeMouseWheel:
      if (sizeof(regame::ClientRelativeMouseWheel) == size) {
        const auto& rmw =
            *reinterpret_cast<const regame::ClientRelativeMouseWheel*>(cc);
        DEBUG_VERBOSE(
            std::format("RelativeMouseWheel: ({}, {})\n", rmw.x, rmw.y));
        switch (mouse_replay_) {
          case MouseReplay::kCgvhid:
            cgvhid_.ReplayRelativeMouseWheel(rmw);
            break;
          case MouseReplay::kSendInput:
            send_input_.ReplayRelativeMouseWheel(rmw);
            break;
        }
      } else {
        DEBUG_PRINT(std::format("Size {} not matched for type {}\n", size,
                                static_cast<int>(cc->type)));
      }
      break;
    default:
      break;
  }
}
