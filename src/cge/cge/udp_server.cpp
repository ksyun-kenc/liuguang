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

#include "udp_server.h"

namespace {

inline void Fail(beast::error_code ec, std::string_view what) {
  std::cerr << "UdpServer# " << what << ": " << ec.message() << '\n';
}

}  // namespace

UdpServer::UdpServer(Engine& engine, udp::endpoint endpoint)
    : engine_(engine), socket_(engine.GetIoContext(), endpoint) {}

void UdpServer::OnRead(const boost::system::error_code& ec,
                       std::size_t bytes_transferred) {
  if (ec) {
    return Fail(ec, "receive_from");
  }

  Read();
}

void UdpServer::OnWrite(const boost::system::error_code& ec,
                        std::size_t bytes_transferred) {}