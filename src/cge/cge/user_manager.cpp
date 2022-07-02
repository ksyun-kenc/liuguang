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

#include "user_manager.h"

#include "app.hpp"

using namespace std::literals::chrono_literals;
using namespace std::literals::string_view_literals;

constexpr std::size_t kMaxRetryTimes = 3;
constexpr auto kRetryInterval = 7s;

constexpr auto kContentType{"application/json; charset=utf-8"sv};
constexpr boost::json::string_view kProtocol{"jsonrpc"};
constexpr boost::json::string_view kProtocolVersion{"2.0"};
constexpr boost::json::string_view kId{"id"};
constexpr boost::json::string_view kMethod{"method"};
constexpr boost::json::string_view kParams{"params"};

namespace {

inline void Fail(boost::system::error_code ec,
                 std::string_view what,
                 const tcp::endpoint& endpoint) {
  APP_ERROR() << "UserManager: " << what << " " << endpoint << " error "
              << ec.value() << ", " << ec.message() << '\n';
}

}  // namespace

void tag_invoke(const json::value_from_tag,
                json::value& root,
                const UserManager::Verification& verification) {
  auto& jo = root.emplace_object();
  jo["version"] = verification.version;
  jo["username"] = verification.username;
  jo["type"] = verification.type;
  jo["data"] = verification.data;
}

UserManager::~UserManager() {
  retry_timer_.cancel();
  keep_alive_timer_.cancel();
  Close();
}

void UserManager::Initialize() {
  request_.version(11);
  request_.method(http::verb::post);
  request_.target(g_app.Engine().GetUserServiceTarget());
  std::stringstream host;
  host << g_app.Engine().GetUserServiceEndpoint();
  request_.set(http::field::content_type, kContentType.data());
  request_.set(http::field::host, host.str());
  request_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
}

void UserManager::Close() {
  if (stream_) {
    if (stream_->socket().is_open()) {
      beast::error_code ec;
      stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
      stream_->close();
    }
  }
  read_buffer_.clear();
  response_.clear();
  response_.body().clear();
  json_parser_.reset();
}

void UserManager::Login(const Verification& verification) {
  username_ = verification.username;

  json::object jo;
  jo[kProtocol] = kProtocolVersion;
  jo[kId] = 0;
  jo[kMethod] = "login";
  jo[kParams] = json::value_from(verification);
  InvokeConnection(Method::kLogin, json::serialize(jo));
}

void UserManager::KeepAlive() {
  if (user_state_ < UserState::kLoggedIn) {
    DEBUG_PRINT("Not logged in!\n");
    return;
  }

  json::object jo;
  jo[kProtocol] = kProtocolVersion;
  jo[kId] = 0;
  jo[kMethod] = "keepalive";
  auto& params = jo[kParams].emplace_object();
  params["session_id"] = session_id_;
  InvokeConnection(Method::kKeepAlive, json::serialize(jo));
}

void UserManager::Logout() {
  if (user_state_ < UserState::kLoggingIn) {
    DEBUG_PRINT("Not logging/logged in!\n");
    return;
  }

  json::object jo;
  jo[kProtocol] = kProtocolVersion;
  // notification, so no id
  jo[kMethod] = "logout";
  auto& params = jo[kParams].emplace_object();
  params["session_id"] = session_id_;
  InvokeConnection(Method::kLogout, json::serialize(jo));
}

void UserManager::InvokeConnection(Method method, std::string request_body) {
  assert(!request_body.empty());
  if (request_body.empty()) {
    return;
  }

  std::lock_guard<std::mutex> lock(request_queue_mutex_);
  request_queue_.emplace(std::pair(method, std::move(request_body)));
  if (1 < request_queue_.size()) {
    return;
  }

  Close();
  stream_ = std::make_unique<beast::tcp_stream>(ioc_);
  if (!stream_) {
    DEBUG_PRINT("Not enough memory!\n");
    return;
  }

  stream_->async_connect(
      g_app.Engine().GetUserServiceEndpoint(),
      beast::bind_front_handler(&UserManager::OnConnect, shared_from_this()));
}

void UserManager::InvokeNextConnection() {
  std::lock_guard<std::mutex> lock(request_queue_mutex_);
  if (!request_queue_.empty()) {
    stream_->async_connect(
        g_app.Engine().GetUserServiceEndpoint(),
        beast::bind_front_handler(&UserManager::OnConnect, shared_from_this()));
  }
}

void UserManager::InvokeNextKeepAlive() {
  if (0 < interval_) {
    keep_alive_timer_.expires_after(std::chrono::seconds(interval_));
    keep_alive_timer_.async_wait([&](const boost::system::error_code& error) {
      if (!error) {
        KeepAlive();
      }
    });
  }
}

void UserManager::RetryNextConnection() {
  retry_timer_.expires_after(kRetryInterval);
  retry_timer_.async_wait([&](const boost::system::error_code& error) {
    if (!error) {
      InvokeNextConnection();
    }
  });
}

void UserManager::OnConnect(beast::error_code ec) {
  assert(!request_queue_.empty());

  if (ec) {
    ++retry_times_;
    if (kMaxRetryTimes < retry_times_) {
      switch (request_queue_.front().first) {
        case Method::kLogin:
          user_state_ = UserState::kNone;
          if (!game_session_.expired()) {
            auto gs = game_session_.lock();
            if (gs) {
              gs->NotifyLoginResult(false);
            }
          }
          break;
        case Method::kKeepAlive:
          if (!game_session_.expired()) {
            auto gs = game_session_.lock();
            if (gs) {
              gs->NotifyKeepAliveResult(false);
            }
          }
          break;
        case Method::kLogout:
          break;
      }
    } else {
      RetryNextConnection();
    }
    return Fail(ec, "connect", g_app.Engine().GetUserServiceEndpoint());
  }
  APP_DEBUG() << "Connected to Regame user service "
              << g_app.Engine().GetUserServiceEndpoint() << '\n';

  std::lock_guard<std::mutex> lock(request_queue_mutex_);
  request_.body() = request_queue_.front().second;
  request_.prepare_payload();
  DEBUG_VERBOSE(std::format("Request: {}\n", request_.body()));
  http::async_write(
      *stream_, request_,
      beast::bind_front_handler(&UserManager::OnRequest, shared_from_this()));
}

void UserManager::OnRequest(beast::error_code ec,
                            std::size_t bytes_transferred) {
  assert(!request_queue_.empty());

  if (ec) {
    switch (request_queue_.front().first) {
      case Method::kLogin:
        if (!game_session_.expired()) {
          auto gs = game_session_.lock();
          if (gs) {
            gs->NotifyLoginResult(false);
          }
        }
        break;
    }
    InvokeNextConnection();
    return Fail(ec, "write", g_app.Engine().GetUserServiceEndpoint());
  }

  switch (request_queue_.front().first) {
    case Method::kLogin:
      user_state_ = UserState::kLoggingIn;
      http::async_read(*stream_, read_buffer_, response_,
                       beast::bind_front_handler(&UserManager::OnLoginResponse,
                                                 shared_from_this()));
      break;
    case Method::kKeepAlive:
      http::async_read(
          *stream_, read_buffer_, response_,
          beast::bind_front_handler(&UserManager::OnKeepAliveResponse,
                                    shared_from_this()));
      break;
    case Method::kLogout:
      user_state_ = UserState::kLoggingOut;
      http::async_read(*stream_, read_buffer_, response_,
                       beast::bind_front_handler(&UserManager::OnLogoutResponse,
                                                 shared_from_this()));
      break;
    default:
      break;
  }
}

void UserManager::OnLoginResponse(beast::error_code ec,
                                  std::size_t bytes_transferred) {
  retry_times_ = 0;
  request_queue_.pop();

  bool authorized = false;
  BOOST_SCOPE_EXIT_ALL(this, &authorized) {
    Close();

    if (authorized) {
      user_state_ = UserState::kLoggedIn;
      InvokeNextKeepAlive();
    }
    if (!game_session_.expired()) {
      auto gs = game_session_.lock();
      if (gs) {
        gs->NotifyLoginResult(authorized);
      }
    }
  };

  if (ec) {
    return Fail(ec, "read", g_app.Engine().GetUserServiceEndpoint());
  }

  if (response_.result() != http::status::ok) {
    return Fail(ec, "http", g_app.Engine().GetUserServiceEndpoint());
  }

  json::error_code json_ec;
  json_parser_.write(response_.body(), json_ec);
  if (json_ec) {
    return Fail(ec, "json", g_app.Engine().GetUserServiceEndpoint());
  }
  json_parser_.finish(json_ec);
  if (json_ec) {
    return Fail(ec, "parse", g_app.Engine().GetUserServiceEndpoint());
  }
  if (json_parser_.done()) {
    retry_times_ = 0;

    json::value jv(json_parser_.release());
    json_parser_.reset();

    if (auto jo = jv.if_object(); nullptr != jo) {
      DEBUG_VERBOSE(std::format("Login: {}\n", json::serialize(*jo)));
      if (auto result_jv = jo->if_contains("result"); nullptr != result_jv) {
        if (auto result_jo = result_jv->if_object(); nullptr != result_jo) {
          if (auto interval_jv = result_jo->if_contains("interval"),
              session_id_jv = result_jo->if_contains("session_id");
              nullptr != interval_jv && nullptr != session_id_jv) {
            if (auto interval = interval_jv->if_int64(); nullptr != interval) {
              if (auto session_id = session_id_jv->if_string();
                  nullptr != session_id) {
                interval_ = *interval;
                session_id_ = *session_id;
                authorized = !session_id_.empty();
              }
            }
          }
        }
      }
    }
  }
}

void UserManager::OnKeepAliveResponse(beast::error_code ec,
                                      std::size_t bytes_transferred) {
  bool failed = true;
  bool kept_alive = false;
  BOOST_SCOPE_EXIT_ALL(this, &failed, &kept_alive) {
    Close();

    if (failed) {
      ++retry_times_;
      if (retry_times_ <= kMaxRetryTimes) {
        RetryNextConnection();
      }
    } else {
      if (kept_alive) {
        InvokeNextKeepAlive();
      }

      if (!game_session_.expired()) {
        auto gs = game_session_.lock();
        if (gs) {
          gs->NotifyKeepAliveResult(kept_alive);
        }
      }
    }
  };

  if (ec) {
    return Fail(ec, "read", g_app.Engine().GetUserServiceEndpoint());
  }

  if (response_.result() != http::status::ok) {
    return Fail(ec, "http", g_app.Engine().GetUserServiceEndpoint());
  }

  json::error_code json_ec;
  json_parser_.write(response_.body(), json_ec);
  if (json_ec) {
    return Fail(json_ec, "json", g_app.Engine().GetUserServiceEndpoint());
  }
  json_parser_.finish(json_ec);
  if (json_ec) {
    return Fail(json_ec, "parse", g_app.Engine().GetUserServiceEndpoint());
  }
  if (json_parser_.done()) {
    retry_times_ = 0;
    failed = false;
    request_queue_.pop();

    json::value jv(json_parser_.release());
    json_parser_.reset();

    if (auto jo = jv.if_object(); nullptr != jo) {
      DEBUG_VERBOSE(std::format("Keepalive: {}\n", json::serialize(*jo)));
      if (auto result_jv = jo->if_contains("result"); nullptr != result_jv) {
        if (auto result_jo = result_jv->if_object(); nullptr != result_jo) {
          if (auto interval_jv = result_jo->if_contains("interval"),
              session_id_jv = result_jo->if_contains("session_id");
              nullptr != interval_jv && nullptr != session_id_jv) {
            if (auto interval = interval_jv->if_int64(); nullptr != interval) {
              if (auto session_id = session_id_jv->if_string();
                  nullptr != session_id) {
                if (session_id_ == *session_id) {
                  interval_ = *interval;
                  kept_alive = true;
                  return;
                }
              }
            }
          }
        }
      }
    }
  }
}

void UserManager::OnLogoutResponse(beast::error_code ec,
                                   std::size_t bytes_transferred) {
  BOOST_SCOPE_EXIT_ALL(this) { Close(); };
  retry_times_ = 0;
  user_state_ = UserState::kNone;
  request_queue_.pop();
}
