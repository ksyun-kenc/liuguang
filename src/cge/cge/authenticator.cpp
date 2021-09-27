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

#include "authenticator.h"

#include "app.hpp"

using namespace std::literals::string_view_literals;

// https://github.com/ksyun-kenc/regame-authenticator
// hardcoded loopback address for security
const tcp::endpoint kRpcServer{net::ip::address_v4::loopback(), 8545};
constexpr boost::beast::string_view kTarget{"/"};
constexpr auto kContentType{"application/json; charset=utf-8"sv};

namespace {

inline void Fail(beast::error_code ec,
                 std::string_view what,
                 const tcp::endpoint& endpoint) {
  APP_ERROR() << "Authenticator: " << what << " " << endpoint << " error "
              << ec.value() << ", " << ec.message() << '\n';
}

}  // namespace

void tag_invoke(const json::value_from_tag,
                json::value& root,
                const Authenticator::Login& login) {
  auto& jo = root.emplace_object();
  jo["version"] = login.version;
  jo["username"] = login.username;
  jo["type"] = login.type;
  jo["data"] = login.data;
}

Authenticator::~Authenticator() {
  Close();
}

void Authenticator::Init() {
  request_.version(11);
  request_.method(http::verb::post);
  request_.target(kTarget);
  std::stringstream host;
  host << kRpcServer;
  request_.set(http::field::content_type, kContentType.data());
  request_.set(http::field::host, host.str());
  request_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
}

void Authenticator::Close() {
  if (stream_.socket().is_open()) {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_both, ec);
    stream_.close();
  }
  read_buffer_.clear();
}

void Authenticator::Verify(const Login& login) {
  json::object jo;
  jo["jsonrpc"] = "2.0";
  jo["id"] = 0;
  jo["method"] = "verify";
  jo["params"] = json::value_from(login);
  request_body_ = json::serialize(jo);
  stream_.async_connect(
      kRpcServer,
      beast::bind_front_handler(&Authenticator::OnConnect, shared_from_this()));
}

void Authenticator::OnConnect(beast::error_code ec) {
  if (ec) {
    if (!game_session_.expired()) {
      auto gs = game_session_.lock();
      gs->SetAuthorized(false);
    }
    return Fail(ec, "connect", kRpcServer);
  }
  APP_INFO() << "Connected to authenticator " << kRpcServer << '\n';
  DEBUG_VERBOSE(std::format("Request: {}\n", request_body_));

  request_.body() = request_body_;
  request_.prepare_payload();
  http::async_write(
      stream_, request_,
      beast::bind_front_handler(&Authenticator::OnWrite, shared_from_this()));
}

void Authenticator::OnRead(beast::error_code ec,
                           std::size_t bytes_transferred) {
  bool authorized = false;
  BOOST_SCOPE_EXIT_ALL(this, &authorized) {
    if (!game_session_.expired()) {
      auto gs = game_session_.lock();
      gs->SetAuthorized(authorized);
    }
  };

  if (ec) {
    return Fail(ec, "read", kRpcServer);
  }

  if (response_.result() != http::status::ok) {
    return Fail(ec, "http", kRpcServer);
  }

  json::error_code e;
  json_parser_.write(response_.body(), e);
  if (e) {
    return Fail(ec, "json", kRpcServer);
  }
  json_parser_.finish(ec);
  if (e) {
    return Fail(ec, "parse", kRpcServer);
  }
  if (json_parser_.done()) {
    json::value result(json_parser_.release());
    json_parser_.reset();
    Close();

    if (result.is_object()) {
      auto& jo = result.as_object();
      DEBUG_VERBOSE(std::format("Authenticator: {}\n", json::serialize(jo)));
      auto result = jo.if_contains("result");
      if (nullptr != result) {
        auto b = result->if_bool();
        authorized = (nullptr != b) && b;
      }
    }
  }
}

void Authenticator::OnWrite(beast::error_code ec,
                            std::size_t bytes_transferred) {
  if (ec) {
    if (!game_session_.expired()) {
      auto gs = game_session_.lock();
      gs->SetAuthorized(false);
    }
    return Fail(ec, "write", kRpcServer);
  }
  Read();
}
