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

class Authenticator : public std::enable_shared_from_this<Authenticator> {
 public:
  Authenticator(net::io_context& ioc, std::weak_ptr<GameSession>&& game_session)
      : ioc_(ioc), game_session_(std::move(game_session)), stream_(ioc) {}
  ~Authenticator();

  void Init();
  void Close();

  struct Login {
    std::int64_t version;
    std::string username;
    std::int64_t type;
    std::string data;
  };
  void Verify(const Login& login);

 private:
  void OnConnect(beast::error_code ec);
  void OnRead(beast::error_code ec, std::size_t bytes_transferred);
  void OnWrite(beast::error_code ec, std::size_t bytes_transferred);

  void Read() {
    http::async_read(
        stream_, read_buffer_, response_,
        beast::bind_front_handler(&Authenticator::OnRead, shared_from_this()));
  }

 private:
  net::io_context& ioc_;
  std::weak_ptr<GameSession> game_session_;
  beast::tcp_stream stream_;
  beast::flat_buffer read_buffer_;
  http::request<http::string_body> request_;
  http::response<http::string_body> response_;
  std::string request_body_;
  json::stream_parser json_parser_;
};
