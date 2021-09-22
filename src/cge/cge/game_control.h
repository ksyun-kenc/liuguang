#pragma once

#include <Xinput.h>

#include "cgvhid_client.h"
#include "regame/protocol.h"

enum class GamepadReplay { kNone = 0, kCgvhid, kVigem };
enum class KeyboardReplay { kNone = 0, kCgvhid };
enum class MouseReplay { kNone = 0, kCgvhid };

class GameService;

class GameControl {
 public:
  GameControl(GameService& game_service) noexcept
      : game_service_(game_service) {}
  ~GameControl() {
    if (KeyboardReplay::kCgvhid == keyboard_replay_) {
      cgvhid_client_.KeyboardReset();
    }
    if (MouseReplay::kCgvhid == mouse_replay_) {
      cgvhid_client_.MouseReset();
    }
  }

  void Initialize() noexcept;
  void Replay(const regame::ClientControl* cc, std::uint32_t size) noexcept;

 private:
  void ReplayGamepadAxis(const regame::ClientGamepadAxis& ga);
  void ReplayGamepadButton(const regame::ClientGamepadButton& gb);
  void ReplayKeyboard(const regame::ClientKeyboard& k);
  void ReplayKeyboardVk(const regame::ClientKeyboard& k);
  void ReplayMouseMove(const regame::ClientMouseMove& mm);
  void ReplayMouseButton(const regame::ClientMouseButton& mb);
  void ReplayMouseWheel(const regame::ClientMouseWheel& mw);

 private:
  GameService& game_service_;
  CgvhidClient cgvhid_client_;

  std::shared_ptr<class ViGEmClient> vigem_client_;
  std::shared_ptr<class ViGEmTargetX360> vigem_target_x360_;
  XINPUT_GAMEPAD gamepad_state_{};

  GamepadReplay gamepad_replay_;
  KeyboardReplay keyboard_replay_;
  MouseReplay mouse_replay_;
  POINT mouse_last_pos_;
};
