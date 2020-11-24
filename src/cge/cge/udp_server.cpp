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

#include "control.h"

namespace {

inline void Fail(beast::error_code ec, std::string_view what) {
  std::cerr << "UdpServer# " << what << ": " << ec.message() << '\n';
}

}  // namespace

UdpServer::UdpServer(Engine& engine,
                     udp::endpoint endpoint,
                     KeyboardReplay keyboard_replay)
    : engine_(engine),
      socket_(engine.GetIoContext(), endpoint),
      keyboard_replay_(keyboard_replay) {
  if (KeyboardReplay::CGVHID == keyboard_replay) {
    cgvhid_client_.Init(0, 0);
    int error_code = cgvhid_client_.KeyboardReset();
    if (0 != error_code) {
      keyboard_replay = KeyboardReplay::NONE;
      std::cerr << "KeyboardReset() failed with " << error_code << '\n';
    }
  }
}

void UdpServer::OnRead(const boost::system::error_code& ec,
                       std::size_t bytes_transferred) {
  if (ec) {
    return Fail(ec, "receive_from");
  }

  if (bytes_transferred >= sizeof(ControlBase)) {
    auto control_element =
        reinterpret_cast<ControlElement*>(recv_buffer_.data());
    switch (static_cast<ControlType>(control_element->base.type)) {
      case ControlType::KEYBOARD:
        if (bytes_transferred < sizeof(ControlKeyboard)) {
          break;
        }
        if (KeyboardReplay::CGVHID == keyboard_replay_) {
          if (ControlKeyboardFlagUp & control_element->keyboard.flags) {
            cgvhid_client_.KeyboardRelease(control_element->keyboard.key_code);
          } else if (ControlKeyboardFlagDown &
                     control_element->keyboard.flags) {
            cgvhid_client_.KeyboardPress(control_element->keyboard.key_code);
          }
        }
        break;
    }
  }
  Read();
}

void UdpServer::OnWrite(const boost::system::error_code& ec,
                        std::size_t bytes_transferred) {}