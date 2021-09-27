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

#include "game_service.h"

#include "app.hpp"

namespace {

inline void Fail(beast::error_code ec, std::string_view what) {
  APP_ERROR() << "GameService: " << what << " error " << ec.value() << ", "
              << ec.message() << '\n';
}
inline void Fail(beast::error_code ec,
                 std::string_view what,
                 const tcp::endpoint& endpoint) {
  APP_ERROR() << "GameService: " << what << " " << endpoint << " error "
              << ec.value() << ", " << ec.message() << '\n';
}

}  // namespace

constexpr size_t kMaxClientCount = 8;

#pragma region "GameService"
GameService::GameService(net::io_context& ioc,
                         const tcp::endpoint& endpoint,
                         const std::vector<uint8_t>& disable_keys,
                         GamepadReplay gamepad_replay,
                         KeyboardReplay keyboard_replay,
                         MouseReplay mouse_replay) noexcept
    : ioc_(ioc),
      acceptor_(ioc),
      gamepad_replay_(gamepad_replay),
      keyboard_replay_(keyboard_replay),
      mouse_replay_(mouse_replay) {
  beast::error_code ec;

  acceptor_.open(endpoint.protocol(), ec);
  if (ec) {
    Fail(ec, "open");
    return;
  }

  acceptor_.set_option(net::socket_base::reuse_address(true), ec);
  if (ec) {
    Fail(ec, "set_option");
    return;
  }

  acceptor_.bind(endpoint, ec);
  if (ec) {
    Fail(ec, "bind");
    return;
  }

  acceptor_.listen(net::socket_base::max_listen_connections, ec);
  if (ec) {
    Fail(ec, "listen");
    return;
  }

  for (const auto& e : disable_keys) {
    disable_keys_[e] = true;
  }
}

void GameService::Accept() {
  acceptor_.async_accept(ioc_, beast::bind_front_handler(&GameService::OnAccept,
                                                         shared_from_this()));
}

bool GameService::Join(std::shared_ptr<GameSession> session) noexcept {
  bool inserted = false;
  {
    std::lock_guard<std::mutex> lock(session_mutex_);
    if (authorized_sessions_.size() < kMaxClientCount) {
      sessions_.insert(session);
      inserted = true;
    }
  }
  if (!inserted) {
    session->Stop(true);
  }
  return inserted;
}

bool GameService::AddAuthorized(std::shared_ptr<GameSession> session) noexcept {
  bool inserted = false;
  bool first = false;
  {
    std::lock_guard<std::mutex> lock(session_mutex_);
    first = authorized_sessions_.size() == 0;
    if (authorized_sessions_.size() < kMaxClientCount) {
      authorized_sessions_.insert(session);
      inserted = true;
    }
  }
  if (inserted) {
    if (first) {
      g_app.GetEngine().EncoderRun();
    }
  } else {
    session->Stop(true);
  }
  return inserted;
}

void GameService::Leave(std::shared_ptr<GameSession> session) noexcept {
  bool last_authorized = false;
  {
    std::lock_guard<std::mutex> lock(session_mutex_);
    sessions_.erase(session);
    if (authorized_sessions_.erase(session) > 0) {
      last_authorized = authorized_sessions_.size() == 0;
    }
  }
  if (last_authorized) {
    g_app.GetEngine().EncoderStop();
  }
}

size_t GameService::Send(std::string buffer) {
  std::lock_guard<std::mutex> lock(session_mutex_);
  size_t count = authorized_sessions_.size();
  if (1 == count) {
    // can move when only one
    authorized_sessions_.begin()->get()->Write(std::move(buffer));
  } else {
    // must copy when > 1
    for (const auto& session : authorized_sessions_) {
      session->Write(buffer);
    }
  }
  return count;
}

void GameService::OnAccept(beast::error_code ec, tcp::socket socket) {
  if (ec) {
    return Fail(ec, "Accept", socket.remote_endpoint());
  }

  APP_INFO() << "Accept " << socket.remote_endpoint() << '\n';

  socket.set_option(tcp::no_delay(true));
  std::make_shared<GameSession>(ioc_, std::move(socket), shared_from_this())
      ->Run();

  Accept();
}

void GameService::Stop(bool restart) {
  beast::error_code ec;
  // acceptor_.cancel(ec);
  acceptor_.close(ec);
  while (!sessions_.empty()) {
    (*sessions_.begin())->Stop(restart);
  }
}

void GameService::CloseAllClients() {
  while (!sessions_.empty()) {
    (*sessions_.begin())->Stop(true);
  }
}
#pragma endregion
