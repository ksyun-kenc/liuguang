#pragma once

#include "net.hpp"

#include "game_control.h"

class GameService;

class GameSession : public std::enable_shared_from_this<GameSession> {
 public:
  explicit GameSession(tcp::socket&& socket,
                       std::shared_ptr<GameService>&& game_service) noexcept
      : ws_(std::move(socket)),
        game_service_(std::move(game_service)),
        remote_endpoint_(ws_.next_layer().socket().remote_endpoint()),
        game_control_(*game_service_.get()) {}

  ~GameSession() = default;

  void Run() {
    game_control_.Initialize();
    net::dispatch(
        ws_.get_executor(),
        beast::bind_front_handler(&GameSession::OnRun, shared_from_this()));
  }

  void Stop(bool restart);

  void Read() {
    ws_.async_read(read_buffer_, beast::bind_front_handler(&GameSession::OnRead,
                                                           shared_from_this()));
  }

  void Write(std::string buffer);

 private:
  void OnRun();
  void OnAccept(beast::error_code ec);
  void OnStop(beast::error_code ec);
  void OnRead(beast::error_code ec, std::size_t bytes_transferred);
  void OnWrite(beast::error_code ec, std::size_t bytes_transferred);
  bool ServeClient();

 private:
  std::shared_ptr<GameService> game_service_;
  bool authorized_ = false;
  websocket::stream<beast::tcp_stream> ws_;
  net::ip::tcp::endpoint remote_endpoint_;
  beast::flat_buffer read_buffer_;

  std::mutex queue_mutex_;
  std::queue<std::string> write_queue_;
  bool is_audio_header_sent_ = false;
  bool is_video_header_sent_ = false;

  std::string username_;

  enum class State { kNone, kHead, kBody } state_ = State::kNone;
  GameControl game_control_;
};
