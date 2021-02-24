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

#include "regame/protocol.h"

using namespace std::literals::chrono_literals;

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

constexpr size_t kMaxClientCount = 8;

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

bool WsServer::AddAuthorized(std::shared_ptr<WsSession> session) noexcept {
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
      engine_.EncoderRun();
    }
  } else {
    session->Stop(true);
  }
  return inserted;
}

void WsServer::Leave(std::shared_ptr<WsSession> session) noexcept {
  bool last_authorized = false;
  {
    std::lock_guard<std::mutex> lock(session_mutex_);
    sessions_.erase(session);
    if (authorized_sessions_.erase(session) > 0) {
      last_authorized = authorized_sessions_.size() == 0;
    }
  }
  if (last_authorized) {
    engine_.EncoderStop();
  }
}

size_t WsServer::Send(std::string buffer) {
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

void WsServer::OnAccept(beast::error_code ec, tcp::socket socket) {
  if (ec) {
    return ListenerFail(ec, socket.remote_endpoint(), "accept");
  }

  std::cout << "Accept " << socket.remote_endpoint() << '\n';

  socket.set_option(tcp::no_delay(true));
  std::make_shared<WsSession>(std::move(socket), shared_from_this())->Run();

  Accept();
}

void WsServer::Stop(bool restart) {
  beast::error_code ec;
  // acceptor_.cancel(ec);
  acceptor_.close(ec);
  while (!sessions_.empty()) {
    (*sessions_.begin())->Stop(restart);
  }
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

void WsSession::Stop(bool restart) {
  if (ws_.is_open()) {
    std::cout << "Closing " << ws_.next_layer().socket().remote_endpoint()
              << '\n';

    if (restart) {
      ws_.async_close(
          websocket::close_reason(websocket::close_code::try_again_later),
          beast::bind_front_handler(&WsSession::OnStop, shared_from_this()));
    } else {
      server_->Leave(shared_from_this());
      ws_.control_callback();
      beast::error_code ec;
      ws_.close(websocket::close_code::going_away, ec);
    }
  } else {
    server_->Leave(shared_from_this());
  }
}

void WsSession::Write(std::string buffer) {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  const auto header = reinterpret_cast<regame::NetPacketHeader*>(buffer.data());
  switch (header->type) {
    case regame::NetPacketType::Audio:
      if (!is_audio_header_sent_) {
        is_audio_header_sent_ = true;
        buffer = Engine::GetInstance().GetAudioHeader() + buffer;
      }
      break;
    case regame::NetPacketType::Video:
      if (!is_video_header_sent_) {
        is_video_header_sent_ = true;
        buffer = Engine::GetInstance().GetVideoHeader() + buffer;
      }
      break;
    default:
      break;
  }
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
    Stop(true);
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
    Stop(true);

    if (ec == websocket::error::closed) {
      return;
    }
    return SessionFail(ec, ws_.next_layer().socket().remote_endpoint(), "read");
  }
#if _DEBUG
  std::cout << __func__ << ": " << bytes_transferred << '\n';
#endif

  if (!ServeClient()) {
    Stop(true);
    return;
  }

  Read();
}

void WsSession::OnWrite(beast::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    Stop(true);
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

bool WsSession::ServeClient() {
  static enum class State { NONE, HEAD, BODY } state = State::NONE;
  static std::chrono::steady_clock::time_point first_byte;

  for (; read_buffer_.size() > 0;) {
    switch (state) {
      case State::NONE:
        first_byte = std::chrono::steady_clock::now();
        state = State::HEAD;
        // pass through
      case State::HEAD:
        if (std::chrono::steady_clock::now() - first_byte > 7s) {
          return false;
        }
        if (read_buffer_.size() < sizeof(regame::NetPacketHeader)) {
          return true;
        }
        state = State::BODY;
        // pass through
      case State::BODY:
        if (std::chrono::steady_clock::now() - first_byte > 7s) {
          return false;
        }
        auto header =
            static_cast<regame::NetPacketHeader*>(read_buffer_.data().data());
        if (regame::kNetPacketCurrentVersion != header->version) {
          read_buffer_.consume(read_buffer_.size());
          state = State::NONE;
          std::cerr << "Invalid packet version\n";
          return false;
        }
        uint32_t body_size = ntohl(header->size);
        uint32_t packet_size = sizeof(regame::NetPacketHeader) + body_size;
        if (read_buffer_.size() < packet_size) {
          return true;
        }

        // whole
        auto body =
            reinterpret_cast<char*>(header) + sizeof(regame::NetPacketHeader);
        auto type = static_cast<regame::NetPacketType>(header->type);
        if (authorized_) {
          if (regame::NetPacketType::Ping == type) {
          }
        } else {
          if (regame::NetPacketType::Login == type) {
            auto login = reinterpret_cast<regame::Login*>(body);
            if (login->verification_size > sizeof(login->verification_data)) {
              return false;
            }
            std::string temp(login->username, sizeof(login->username));
            username_.assign(temp.data());
            if (regame::VerificationType::Code == login->verification_type) {
              std::string password(login->verification_data,
                                   login->verification_size);
              // TO-DO
              authorized_ = username_ == "UMU" && password == "123456";
            }
          }

          if (!authorized_) {
            std::cout << username_ << " login failed from "
                      << ws_.next_layer().socket().remote_endpoint() << '\n';
            return false;
          }

          if (!server_->AddAuthorized(shared_from_this())) {
            return false;
          }

          std::string buffer;
          buffer.resize(sizeof(regame::NetPacketLoginResult));
          auto& login_result =
              *reinterpret_cast<regame::NetPacketLoginResult*>(buffer.data());
          login_result.header.version = regame::kNetPacketCurrentVersion;
          login_result.header.type = regame::NetPacketType::Login;
          login_result.header.size = htonl(sizeof(login_result.login_result));
          login_result.login_result.error_code = htonl(0);
          login_result.login_result.audio_codec =
              htonl(Engine::GetInstance().GetAudioCodecID());
          login_result.login_result.video_codec =
              htonl(Engine::GetInstance().GetVideoCodecID());
          Write(std::move(buffer));

          std::cout << "Authorized "
                    << ws_.next_layer().socket().remote_endpoint() << '\n';
        }

        read_buffer_.consume(packet_size);
        state = State::NONE;
        break;
    }
  }
  return true;
}
#pragma endregion
