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

#include "net.hpp"

#include "game_control.h"

class GameService;
class UserManager;

class GameSession : public std::enable_shared_from_this<GameSession> {
 public:
  explicit GameSession(net::io_context& ioc,
                       tcp::socket&& socket,
                       std::shared_ptr<GameService>&& game_service) noexcept
      : ioc_(ioc),
        ws_(std::move(socket)),
        game_service_(std::move(game_service)),
        remote_endpoint_(ws_.next_layer().socket().remote_endpoint()),
        game_control_(*game_service_.get()) {}

  ~GameSession() = default;

  void Run() {
    net::dispatch(ioc_, beast::bind_front_handler(&GameSession::OnRun,
                                                  shared_from_this()));
  }

  void Stop(bool restart);

  void Read() {
    ws_.async_read(read_buffer_, beast::bind_front_handler(&GameSession::OnRead,
                                                           shared_from_this()));
  }

  void Write(std::string buffer);

  void NotifyLoginResult(bool result) {
    net::dispatch(ioc_, beast::bind_front_handler(&GameSession::OnLogin,
                                                  shared_from_this(), result));
  }
  void NotifyKeepAliveResult(bool result) {
    net::dispatch(ioc_, beast::bind_front_handler(&GameSession::OnKeepAlive,
                                                  shared_from_this(), result));
  }

 private:
  void OnRun();
  void OnAccept(beast::error_code ec);
  void OnLogin(bool result) noexcept;
  void OnKeepAlive(bool result) noexcept;
  void OnStop(beast::error_code ec);
  void OnRead(beast::error_code ec, std::size_t bytes_transferred);
  void OnWrite(beast::error_code ec, std::size_t bytes_transferred);
  bool ServeClient();
  bool ServeClientLogin(const regame::ClientPacketHead* client_packet,
                        std::uint32_t packet_size);

 private:
  net::io_context& ioc_;
  std::shared_ptr<GameService> game_service_;
  websocket::stream<beast::tcp_stream> ws_;
  net::ip::tcp::endpoint remote_endpoint_;
  beast::flat_buffer read_buffer_;

  std::mutex queue_mutex_;
  std::queue<std::string> write_queue_;
  bool is_audio_header_sent_ = false;
  bool is_video_header_sent_ = false;
  bool is_video_keyframe_sent_ = false;

  std::shared_ptr<UserManager> user_manager_;

  enum class ParseState {
    kNone,
    kHead,
    kBody
  } parse_state_ = ParseState::kNone;
  enum class SessionState {
    kNone = 0,
    kAuthorizing,
    kFailed,
    kAuthorized
  } session_state_ = SessionState::kNone;

  GameControl game_control_;
};
