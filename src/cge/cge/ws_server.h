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

#include "net.hpp"

#include "engine.h"

class WsServer;

class WsSession : public std::enable_shared_from_this<WsSession> {
 public:
  explicit WsSession(tcp::socket&& socket,
                     std::shared_ptr<WsServer>&& server) noexcept
      : ws_(std::move(socket)), server_(std::move(server)) {}

  ~WsSession() = default;

  void Run() {
    net::dispatch(
        ws_.get_executor(),
        beast::bind_front_handler(&WsSession::OnRun, shared_from_this()));
  }

  void Stop();

  void Read() {
    ws_.async_read(read_buffer_, beast::bind_front_handler(&WsSession::OnRead,
                                                           shared_from_this()));
  }

  void Write(std::string&& buffer);

 private:
  void OnRun();
  void OnAccept(beast::error_code ec);
  void OnStop(beast::error_code ec);
  void OnRead(beast::error_code ec, std::size_t bytes_transferred);
  void OnWrite(beast::error_code ec, std::size_t bytes_transferred);

 private:
  std::shared_ptr<WsServer> server_;
  bool authorized_ = false;
  websocket::stream<beast::tcp_stream> ws_;
  beast::flat_buffer read_buffer_;

  std::mutex queue_mutex_;
  std::queue<std::string> write_queue_;
};

class WsServer : public std::enable_shared_from_this<WsServer> {
 public:
  WsServer(Engine& engine, const tcp::endpoint& endpoint) noexcept;
  ~WsServer() = default;
  void Run() { Accept(); }
  size_t Send(std::string&& buffer);

 private:
  void Accept() {
    acceptor_.async_accept(
        net::make_strand(engine_.GetIoContext()),
        beast::bind_front_handler(&WsServer::OnAccept, shared_from_this()));
  }

  void OnAccept(beast::error_code ec, tcp::socket socket);

  bool Join(std::shared_ptr<WsSession> session) noexcept;
  void Leave(std::shared_ptr<WsSession> session) noexcept;

  friend class WsSession;

 private:
  Engine& engine_;
  tcp::acceptor acceptor_;

  std::mutex session_mutex_;
  std::unordered_set<std::shared_ptr<WsSession>> sessions_;
};