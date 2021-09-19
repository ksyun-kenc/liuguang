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

#include <unordered_set>

#include "game_session.h"

class GameService : public std::enable_shared_from_this<GameService> {
 public:
  GameService(const tcp::endpoint& endpoint,
              const std::vector<uint8_t>& disable_keys,
              GamepadReplay gamepad_replay,
              KeyboardReplay keyboard_replay,
              MouseReplay mouse_replay) noexcept;
  ~GameService() = default;
  void Run() { Accept(); }
  void Stop(bool restart);
  size_t Send(std::string buffer);
  void CloseAllClients();

  const std::array<bool, 256>& GetDisableKeys() const noexcept {
    return disable_keys_;
  }
  GamepadReplay GetGamepadReplay() const noexcept { return gamepad_replay_; }
  KeyboardReplay GetKeyboardReplay() const noexcept { return keyboard_replay_; }
  MouseReplay GetMouseReplay() const noexcept { return mouse_replay_; }

 private:
  void Accept();

  void OnAccept(beast::error_code ec, tcp::socket socket);

  bool Join(std::shared_ptr<GameSession> session) noexcept;
  void Leave(std::shared_ptr<GameSession> session) noexcept;

  bool AddAuthorized(std::shared_ptr<GameSession> session) noexcept;

  friend class GameSession;

 private:
  tcp::acceptor acceptor_;

  std::array<bool, 256> disable_keys_{};
  GamepadReplay gamepad_replay_;
  KeyboardReplay keyboard_replay_;
  MouseReplay mouse_replay_;

  std::mutex session_mutex_;
  std::unordered_set<std::shared_ptr<GameSession>> sessions_;
  std::unordered_set<std::shared_ptr<GameSession>> authorized_sessions_;
};