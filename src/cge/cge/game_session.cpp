#include "pch.h"

#include "game_session.h"

#include "engine.h"
#include "game_service.h"

using namespace std::literals::chrono_literals;

namespace {

inline void SessionFail(beast::error_code ec,
                        const tcp::endpoint& endpoint,
                        std::string_view what) {
  APP_ERROR() << "GameSession " << endpoint << " " << what << ": "
              << "#" << ec.value() << ", " << ec.message() << '\n';
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
    }
  } else {
    game_service_->Leave(shared_from_this());
  }
}

void GameSession::Write(std::string buffer) {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  const auto server_data = reinterpret_cast<regame::ServerPacket*>(
      buffer.data() + sizeof(regame::PackageHead));
  switch (server_data->action) {
    case regame::ServerAction::kAudio:
      if (!is_audio_header_sent_) {
        is_audio_header_sent_ = true;
        buffer = Engine::GetInstance().GetAudioHeader() + buffer;
      }
      break;
    case regame::ServerAction::kVideo:
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
      beast::bind_front_handler(&GameSession::OnWrite, shared_from_this()));
}

void GameSession::OnAccept(beast::error_code ec) {
  if (ec == websocket::error::closed) {
    APP_INFO() << "Close " << remote_endpoint_ << '\n';
    return;
  }
  if (ec) {
    return SessionFail(ec, remote_endpoint_, "accept");
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

void GameSession::OnStop(beast::error_code ec) {
  game_service_->Leave(shared_from_this());
}

void GameSession::OnRead(beast::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    Stop(true);

    if (ec == websocket::error::closed) {
      return;
    }
    return SessionFail(ec, remote_endpoint_, "read");
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
    return SessionFail(ec, remote_endpoint_, "write");
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
    switch (state_) {
      case State::kNone:
        first_byte = std::chrono::steady_clock::now();
        state_ = State::kHead;
        // pass through
      case State::kHead:
        if (std::chrono::steady_clock::now() - first_byte > 7s) {
          DEBUG_PRINT("Head timeout\n");
          return false;
        }
        if (read_buffer_.size() < sizeof(regame::PackageHead)) {
          return true;
        }
        state_ = State::kBody;
        // pass through
      case State::kBody:
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
            reinterpret_cast<const regame::ClientPacket*>(head + 1);
        auto action = client_packet->action;
        if (authorized_) {
          if (regame::ClientAction::kPing == action) {
          } else if (regame::ClientAction::kControl == action) {
            game_control_.Replay(
                reinterpret_cast<const regame::ClientControl*>(client_packet),
                packet_size);
          }
        } else {
          if (regame::ClientAction::kLogin == action) {
            auto login =
                reinterpret_cast<const regame::ClientLogin*>(client_packet);
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
            APP_ERROR() << username_ << " login failed from "
                        << remote_endpoint_ << '\n';
            return false;
          }

          if (!game_service_->AddAuthorized(shared_from_this())) {
            return false;
          }

          std::string buffer;
          buffer.resize(sizeof(regame::PackageHead) +
                        sizeof(regame::ServerLoginResult));
          auto head = reinterpret_cast<regame::PackageHead*>(buffer.data());
          head->size = htonl(sizeof(regame::ServerLoginResult));
          auto& login_result =
              *reinterpret_cast<regame::ServerLoginResult*>(head + 1);
          login_result.action = regame::ServerAction::kLoginResult;
          login_result.protocol_version = regame::kProtocolVersion;
          login_result.error_code = htonl(0);
          login_result.audio_codec =
              htonl(Engine::GetInstance().GetAudioCodecID());
          login_result.video_codec =
              htonl(Engine::GetInstance().GetVideoCodecID());
          Write(std::move(buffer));

          APP_INFO() << "Authorized " << remote_endpoint_ << '\n';
        }

        read_buffer_.consume(total_size);
        state_ = State::kNone;
        break;
    }
  }
  return true;
}
#pragma endregion