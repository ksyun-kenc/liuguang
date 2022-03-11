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

#include <boost/json.hpp>

#include "game_session.h"

namespace json = boost::json;

class UserManager : public std::enable_shared_from_this<UserManager> {
 public:
  UserManager(net::io_context& ioc, std::weak_ptr<GameSession>&& game_session)
      : ioc_(ioc),
        game_session_(std::move(game_session)),
        retry_timer_(ioc),
        keep_alive_timer_(ioc) {}
  ~UserManager();

  void Initialize();
  void Close();

  const std::string& GetUsername() noexcept { return username_; }

  struct Verification {
    std::int64_t version;
    std::string username;
    std::int64_t type;
    std::string data;
  };
  void Login(const Verification& verification);
  void KeepAlive();
  void Logout();

 private:
  enum class Method { kLogin, kKeepAlive, kLogout };
  enum class UserState {
    kNone,
    kLoggingIn,
    kLoggedIn,
    kLoggingOut
  } user_state_ = UserState::kNone;

  void InvokeConnection(Method method, std::string request_body);
  void InvokeNextConnection();
  void InvokeNextKeepAlive();
  void RetryNextConnection();

  void OnConnect(beast::error_code ec);
  void OnRequest(beast::error_code ec, std::size_t bytes_transferred);
  void OnLoginResponse(beast::error_code ec, std::size_t bytes_transferred);
  void OnKeepAliveResponse(beast::error_code ec, std::size_t bytes_transferred);
  void OnLogoutResponse(beast::error_code ec, std::size_t bytes_transferred);

 private:
  net::io_context& ioc_;
  std::weak_ptr<GameSession> game_session_;
  std::unique_ptr<beast::tcp_stream> stream_{};
  beast::flat_buffer read_buffer_;
  http::request<http::string_body> request_;
  http::response<http::string_body> response_;

  std::mutex request_queue_mutex_;
  std::queue<std::pair<Method, std::string>> request_queue_;

  json::stream_parser json_parser_;

  std::string username_;
  std::string session_id_;
  std::uint64_t interval_;
  std::size_t retry_times_{0};
  boost::asio::steady_timer retry_timer_;
  boost::asio::steady_timer keep_alive_timer_;
};
