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

#include "ws_server.h"

struct ProtocolHeader {
  uint32_t type : 8;
  uint32_t ts : 24;
  uint32_t size;
  float elapsed;
};

namespace {

inline void ListenerFail(beast::error_code ec, std::string_view what) {
  std::cerr << "WsListener# " << what << ": " << ec.message() << '\n';
}
inline void ListenerFail(beast::error_code ec,
                         const tcp::endpoint& endpoint,
                         std::string_view what) {
  std::cerr << "WsListener# " << endpoint << " " << what << ": " << ec.message()
            << '\n';
}

inline void SessionFail(beast::error_code ec,
                        const tcp::endpoint& endpoint,
                        std::string_view what) {
  std::cerr << "WsSession# " << endpoint << " " << what << ": "
            << "(#" << ec.value() << ')' << ec.message() << '\n';
}

}  // namespace

#pragma region "WsServer"
WsServer::WsServer(Engine& engine, const tcp::endpoint& endpoint) noexcept
    : engine_(engine), acceptor_(engine.GetIoContext()) {
  beast::error_code ec;

  if (acceptor_.open(endpoint.protocol(), ec)) {
    ListenerFail(ec, "open");
    return;
  }

  if (acceptor_.set_option(net::socket_base::reuse_address(true), ec)) {
    ListenerFail(ec, "set_option");
    return;
  }

  if (acceptor_.bind(endpoint, ec)) {
    ListenerFail(ec, "bind");
    return;
  }

  if (acceptor_.listen(net::socket_base::max_listen_connections, ec)) {
    ListenerFail(ec, "listen");
    return;
  }
}

bool WsServer::Join(std::shared_ptr<WsSession> session) noexcept {
  bool inserted = false;
  bool first = false;
  {
    std::lock_guard<std::mutex> lock(session_mutex_);
    first = sessions_.size() == 0;
    if (first) {
      sessions_.insert(session);
      inserted = true;
    }
  }
  if (first) {
    engine_.EncoderRun();
  } else {
    // Only one websocket client
    session->Stop();
  }
  return inserted;
}

void WsServer::Leave(std::shared_ptr<WsSession> session) noexcept {
  bool last = false;
  {
    std::lock_guard<std::mutex> lock(session_mutex_);
    if (sessions_.erase(session) > 0) {
      last = sessions_.size() == 0;
    }
  }
  if (last) {
    engine_.EncoderStop();
  }
}

size_t WsServer::Send(std::string&& buffer) {
  std::lock_guard<std::mutex> lock(session_mutex_);
  for (const auto& session : sessions_) {
    // Only one websocket client
    session->Write(std::move(buffer));
    return 1;
  }
  return 0;
}

void WsServer::OnAccept(beast::error_code ec, tcp::socket socket) {
  if (ec) {
    return ListenerFail(ec, socket.remote_endpoint(), "accept");
  }

  std::cout << "Accept " << socket.remote_endpoint() << '\n';

  socket.set_option(tcp::no_delay(true));
  std::make_shared<WsSession>(std::move(socket), shared_from_this())->Run();

  Accept();
}
#pragma endregion

#pragma region "WsSession"
void WsSession::OnRun() {
  ws_.binary(true);
  ws_.set_option(
      websocket::stream_base::timeout::suggested(beast::role_type::server));
  ws_.set_option(
      websocket::stream_base::decorator([](websocket::response_type& res) {
        res.set(http::field::sec_websocket_protocol, "webgame");
      }));

  ws_.async_accept(
      beast::bind_front_handler(&WsSession::OnAccept, shared_from_this()));
}

void WsSession::Stop() {
  if (ws_.is_open()) {
    std::cout << "Closing " << ws_.next_layer().socket().remote_endpoint()
              << '\n';
    ws_.async_close(
        websocket::close_reason(websocket::close_code::try_again_later),
        beast::bind_front_handler(&WsSession::OnAccept, shared_from_this()));
  }
}

void WsSession::Write(std::string&& buffer) {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  write_queue_.emplace(buffer);

  if (write_queue_.size() > 1) {
    return;
  }

  ws_.async_write(
      net::buffer(write_queue_.front()),
      beast::bind_front_handler(&WsSession::OnWrite, shared_from_this()));
}

void WsSession::OnAccept(beast::error_code ec) {
  if (ec == websocket::error::closed) {
    std::cout << "Close " << ws_.next_layer().socket().remote_endpoint()
              << '\n';
    return;
  }
  if (ec) {
    return SessionFail(ec, ws_.next_layer().socket().remote_endpoint(),
                       "accept");
  }

  if (!server_->Join(shared_from_this())) {
    return;
  }

#if _DEBUG
  std::cout << __func__ << "\n";
#endif

  // Write("Hello world!");
  Read();
}

void WsSession::OnStop(beast::error_code ec) {
  server_->Leave(shared_from_this());
}

void WsSession::OnRead(beast::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    server_->Leave(shared_from_this());

    if (ec == websocket::error::closed) {
      return;
    }
    return SessionFail(ec, ws_.next_layer().socket().remote_endpoint(), "read");
  }
#if _DEBUG
  std::cout << __func__ << ": " << bytes_transferred << '\n';
#endif

  Read();
}

void WsSession::OnWrite(beast::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    server_->Leave(shared_from_this());
    return SessionFail(ec, ws_.next_layer().socket().remote_endpoint(),
                       "write");
  }
#if _DEBUG
  if (bytes_transferred != write_queue_.front().size()) {
    std::cout << "bytes_transferred: " << bytes_transferred
              << ", size: " << write_queue_.front().size() << '\n';
  }
#endif
  std::lock_guard<std::mutex> lock(queue_mutex_);
  write_queue_.pop();
  if (!write_queue_.empty()) {
    ws_.async_write(
        net::buffer(write_queue_.front()),
        beast::bind_front_handler(&WsSession::OnWrite, shared_from_this()));
  }
}
#pragma endregion
