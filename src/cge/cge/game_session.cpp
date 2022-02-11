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

#include "game_session.h"

#include "app.hpp"
#include "game_service.h"
#include "user_manager.h"

#define USER_MANAGER 1

using namespace std::literals::chrono_literals;

namespace {

inline void Fail(beast::error_code ec,
                 std::string_view what,
                 const tcp::endpoint& endpoint) {
  APP_ERROR() << "GameSession: " << what << " " << endpoint << " error "
              << ec.value() << ", " << ec.message() << '\n';
}

}  // namespace

#pragma region "GameSession"
void GameSession::OnRun() {
  ws_.binary(true);
  ws_.set_option(
      websocket::stream_base::timeout::suggested(beast::role_type::server));
  ws_.set_option(
      websocket::stream_base::decorator([](websocket::response_type& res) {
        res.set(http::field::sec_websocket_protocol, "webgame");
      }));

  ws_.async_accept(
      beast::bind_front_handler(&GameSession::OnAccept, shared_from_this()));
}

void GameSession::Stop(bool restart) {
  if (user_manager_) {
    user_manager_->Logout();
  }

  if (ws_.is_open()) {
    APP_INFO() << "Closing " << remote_endpoint_ << '\n';
    if (restart) {
      ws_.async_close(
          websocket::close_reason(websocket::close_code::try_again_later),
          beast::bind_front_handler(&GameSession::OnStop, shared_from_this()));
    } else {
      game_service_->Leave(shared_from_this());
      ws_.control_callback();
      beast::error_code ec;
      ws_.close(websocket::close_code::going_away, ec);
      APP_INFO() << "Closed " << remote_endpoint_ << '\n';
    }
  } else {
    game_service_->Leave(shared_from_this());
  }
}

void GameSession::Write(std::string buffer) {
  if (buffer.empty()) {
    return;
  }
  std::lock_guard<std::mutex> lock(queue_mutex_);
  const auto server_data = reinterpret_cast<regame::ServerPacketHead*>(
      buffer.data() + sizeof(regame::PackageHead));
  switch (server_data->action) {
    case regame::ServerAction::kAudio:
      if (!is_audio_header_sent_) {
        is_audio_header_sent_ = true;
        buffer = g_app.Engine().GetAudioHeader() + buffer;
      }
      break;
    case regame::ServerAction::kVideo:
      if (!is_video_header_sent_) {
        is_video_header_sent_ = true;
        buffer = g_app.Engine().GetVideoHeader() + buffer;
      }
      break;
    default:
      break;
  }
  write_queue_.emplace(buffer);

  if (1 < write_queue_.size()) {
    return;
  }

  ws_.async_write(
      net::buffer(write_queue_.front()),
      beast::bind_front_handler(&GameSession::OnWrite, shared_from_this()));
}

void GameSession::OnAccept(beast::error_code ec) {
  if (ec == websocket::error::closed) {
    APP_INFO() << "Close " << remote_endpoint_ << '\n';
    return;
  }
  if (ec) {
    return Fail(ec, "accept", remote_endpoint_);
  }

  if (!game_service_->Join(shared_from_this())) {
    Stop(true);
    return;
  }

#if _DEBUG
  APP_TRACE() << __func__ << "\n";
#endif

  // Write("Hello world!");
  Read();
}

void GameSession::OnLogin(bool result) noexcept {
  if (!result) {
    session_state_ = SessionState::kFailed;
    Stop(true);
    APP_ERROR() << user_manager_->GetUsername() << " from " << remote_endpoint_
                << " login failed !\n";
    return;
  }

  session_state_ = SessionState::kAuthorized;
  if (!game_service_->AddAuthorized(shared_from_this())) {
    return;
  }

  std::string buffer;
  buffer.resize(sizeof(regame::PackageHead) +
                sizeof(regame::ServerLoginResult));
  auto head = reinterpret_cast<regame::PackageHead*>(buffer.data());
  head->size = htonl(sizeof(regame::ServerLoginResult));
  auto& login_result = *reinterpret_cast<regame::ServerLoginResult*>(head + 1);
  login_result.head.action = regame::ServerAction::kLoginResult;
  login_result.protocol_version = regame::kProtocolVersion;
  login_result.error_code = htonl(0);
  login_result.audio_codec = htonl(g_app.Engine().GetAudioCodecID());
  login_result.video_codec = htonl(g_app.Engine().GetVideoCodecID());
  Write(std::move(buffer));

  APP_INFO() << "Authorized " << user_manager_->GetUsername() << " from "
             << remote_endpoint_ << '\n';
  game_control_.Initialize();

  g_app.Engine().VideoProduceKeyframe();
}

void GameSession::OnKeepAlive(bool result) noexcept {
  if (!result) {
    session_state_ = SessionState::kFailed;
    Stop(true);
    APP_ERROR() << user_manager_->GetUsername() << " from "
                << remote_endpoint_ << " keepalive failed!\n";
    return;
  }
}

void GameSession::OnStop(beast::error_code ec) {
  game_service_->Leave(shared_from_this());
  APP_INFO() << "Async closed " << remote_endpoint_ << '\n';
}

void GameSession::OnRead(beast::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    Stop(true);

    if (ec == websocket::error::closed) {
      return;
    }
    return Fail(ec, "read", remote_endpoint_);
  }
#if _DEBUG
  APP_TRACE() << __func__ << ": " << bytes_transferred << '\n';
#endif

  if (!ServeClient()) {
    Stop(true);
    return;
  }

  Read();
}

void GameSession::OnWrite(beast::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    Stop(true);
    return Fail(ec, "write", remote_endpoint_);
  }
#if _DEBUG
  if (bytes_transferred != write_queue_.front().size()) {
    APP_TRACE() << "bytes_transferred: " << bytes_transferred
                << ", size: " << write_queue_.front().size() << '\n';
  }
#endif
  std::lock_guard<std::mutex> lock(queue_mutex_);
  write_queue_.pop();
  if (!write_queue_.empty()) {
    ws_.async_write(
        net::buffer(write_queue_.front()),
        beast::bind_front_handler(&GameSession::OnWrite, shared_from_this()));
  }
}

bool GameSession::ServeClient() {
  static std::chrono::steady_clock::time_point first_byte;

  for (; read_buffer_.size() > 0;) {
    switch (parse_state_) {
      case ParseState::kNone:
        first_byte = std::chrono::steady_clock::now();
        parse_state_ = ParseState::kHead;
        // pass through
      case ParseState::kHead:
        if (std::chrono::steady_clock::now() - first_byte > 7s) {
          DEBUG_PRINT("Head timeout\n");
          return false;
        }
        if (read_buffer_.size() < sizeof(regame::PackageHead)) {
          return true;
        }
        parse_state_ = ParseState::kBody;
        // pass through
      case ParseState::kBody:
        if (std::chrono::steady_clock::now() - first_byte > 7s) {
          DEBUG_PRINT("Body timeout\n");
          return false;
        }
        auto head =
            static_cast<const regame::PackageHead*>(read_buffer_.data().data());
        std::uint32_t packet_size = ntohl(head->size);
        std::uint32_t total_size = sizeof(regame::PackageHead) + packet_size;
        if (read_buffer_.size() < total_size) {
          return true;
        }

        // whole
        auto client_packet =
            reinterpret_cast<const regame::ClientPacketHead*>(head + 1);
        auto action = client_packet->action;
        if (SessionState::kAuthorized <= session_state_) {
          if (regame::ClientAction::kPing == action) {
          } else if (regame::ClientAction::kControl == action) {
            game_control_.Replay(
                reinterpret_cast<const regame::ClientControl*>(client_packet),
                packet_size);
          }
        } else if (SessionState::kNone == session_state_) {
          if (regame::ClientAction::kLogin == action) {
            session_state_ = SessionState::kAuthorizing;
            auto cl =
                reinterpret_cast<const regame::ClientLogin*>(client_packet);
            if (cl->verification_size > sizeof(cl->verification_data)) {
              return false;
            }

            UserManager::Verification verification;
            verification.version =
                static_cast<std::int64_t>(cl->protocol_version);
            verification.username.assign(cl->username, sizeof(cl->username));
            verification.username.resize(
                decltype(verification.username)::traits_type::length(
                    verification.username.data()));
            verification.type =
                static_cast<std::int64_t>(cl->verification_type);
            verification.data.assign(cl->verification_data,
                                     cl->verification_size);
#if USER_MANAGER
            user_manager_ = std::make_shared<UserManager>(
                ioc_, std::move(weak_from_this()));
            if (user_manager_) {
              user_manager_->Init();
              user_manager_->Login(verification);
            }
#else
            // You should remove this backdoor.
            bool authorized = false;
            if (regame::VerificationType::Code == cl->verification_type) {
              authorized = login.username == "UMU" && login.data == "123456";
            }
            SetAuthorized(authorized);
#endif
          }
        }

        read_buffer_.consume(total_size);
        parse_state_ = ParseState::kNone;
        break;
    }
  }
  return true;
}
#pragma endregion
