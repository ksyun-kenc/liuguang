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

#include "cgvhid_client/cgvhid_client.h"
#include "regame/protocol.h"
#include "vigem_client.h"

enum class GamepadReplay { kNone = 0, kCgvhid, kVigem };
enum class KeyboardReplay { kNone = 0, kCgvhid, kSendInput, kMessage };
enum class MouseReplay { kNone = 0, kCgvhid, kSendInput, kMessage };

class GameService;

class GameControl {
 public:
  GameControl(GameService& game_service) noexcept
      : game_service_(game_service) {}
  ~GameControl() {
    if (KeyboardReplay::kCgvhid == keyboard_replay_) {
      cgvhid_.client_.KeyboardReset();
    }
    if (MouseReplay::kCgvhid == mouse_replay_) {
      cgvhid_.client_.MouseReset();
    }
  }

  static void SetDisableKeys(
      const std::vector<uint8_t>& disable_keys) noexcept {
    for (auto& key : disable_keys_) {
      key = false;
    }
    for (const auto& e : disable_keys) {
      disable_keys_[e] = true;
    }
  }

  static bool IsDisableKeys(std::uint8_t key) noexcept {
    return disable_keys_[key];
  }

  void Initialize() noexcept;
  void Replay(const regame::ClientControl* cc, std::uint32_t size) noexcept;

 private:
  struct Cgvhid {
    void ReplayKeyboard(const regame::ClientKeyboard& k);
    void ReplayKeyboardVk(const regame::ClientKeyboard& k);

    void ReplayMouseMove(const regame::ClientMouseMove& mm);
    void ReplayMouseButton(const regame::ClientMouseButton& mb);
    void ReplayMouseWheel(const regame::ClientMouseWheel& mw);

    CgvhidClient client_;
  } cgvhid_;

  struct Message {
    void ReplayKeyboard(const regame::ClientKeyboard& k);
    void ReplayKeyboardVk(const regame::ClientKeyboard& k);

    void ReplayMouseMove(const regame::ClientMouseMove& mm);
    void ReplayMouseButton(const regame::ClientMouseButton& mb);
    void ReplayMouseWheel(const regame::ClientMouseWheel& mw);

    UINT SdlMouseButtonToMessage(std::uint8_t sdl_button,
                                 regame::ButtonState state);

    WPARAM button_state_{0};
  } message_;

  struct SendInput {
    void ReplayKeyboard(const regame::ClientKeyboard& k);
    void ReplayKeyboardVk(const regame::ClientKeyboard& k);

    void ReplayMouseMove(const regame::ClientMouseMove& mm);
    void ReplayMouseButton(const regame::ClientMouseButton& mb);
    void ReplayMouseWheel(const regame::ClientMouseWheel& mw);
  } send_input_;

  struct ViGEm {
    void ReplayGamepadAxis(const regame::ClientGamepadAxis& ga);
    void ReplayGamepadButton(const regame::ClientGamepadButton& gb);

    std::shared_ptr<ViGEmClient> client_;
    std::shared_ptr<ViGEmTargetX360> target_x360_;
    XINPUT_GAMEPAD gamepad_state_{};
  } vigem_;

 private:
  static std::array<bool, 256> disable_keys_;
  static POINT mouse_last_pos_;

  GameService& game_service_;

  GamepadReplay gamepad_replay_;
  KeyboardReplay keyboard_replay_;
  MouseReplay mouse_replay_;
};
