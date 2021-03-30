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

#include <Xinput.h>

#include <boost/bind/bind.hpp>

#include "net.hpp"

#include "cgvhid_client.h"
#include "engine.h"
#include "regame/control.h"

class UdpServer : public std::enable_shared_from_this<UdpServer> {
 public:
  UdpServer(Engine& engine,
            udp::endpoint endpoint,
            std::vector<uint8_t> disable_keys,
            KeyboardReplay keyboard_replay,
            GamepadReplay gamepad_replay);
  ~UdpServer();

  void Run() { Read(); }

  void Stop() noexcept {
    boost::system::error_code ec;
    // socket_.cancel(ec);
    socket_.close(ec);
  }

 private:
  void Read() {
    socket_.async_receive_from(
        net::buffer(recv_buffer_), remote_endpoint_,
        boost::bind(&UdpServer::OnRead, shared_from_this(),
                    net::placeholders::error,
                    net::placeholders::bytes_transferred));
  }

  void OnRead(const boost::system::error_code& ec,
              std::size_t bytes_transferred);

  void OnWrite(const boost::system::error_code& ec,
               std::size_t bytes_transferred);

  void OnControlEvent(std::size_t bytes_transferred) noexcept;

  void OnKeyboardEvent(std::size_t bytes_transferred,
                       ControlElement* control_element) noexcept;
  void OnKeyboardVkEvent(std::size_t bytes_transferred,
                         ControlElement* control_element) noexcept;
  void OnJoystickAxisEvent(std::size_t bytes_transferred,
                           ControlElement* control_element) noexcept;
  void OnJoystickButtonEvent(std::size_t bytes_transferred,
                             ControlElement* control_element) noexcept;
  void OnJoystickHatEvent(std::size_t bytes_transferred,
                          ControlElement* control_element) noexcept;
  void OnGamepadAxisEvent(std::size_t bytes_transferred,
                          ControlElement* control_element) noexcept;
  void OnGamepadButtonEvent(std::size_t bytes_transferred,
                            ControlElement* control_element) noexcept;

 private:
  Engine& engine_;
  udp::socket socket_;
  udp::endpoint remote_endpoint_;
  std::array<char, 65536> recv_buffer_{};

  std::array<bool, 256> disable_keys_{};
  KeyboardReplay keyboard_replay_;
  CgvhidClient cgvhid_client_;

  GamepadReplay gamepad_replay_;
  std::shared_ptr<class ViGEmClient> vigem_client_;
  std::shared_ptr<class ViGEmTargetX360> vigem_target_x360_;
  XINPUT_GAMEPAD gamepad_state_{};
};
