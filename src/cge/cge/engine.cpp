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

#include "engine.h"

#include "app.hpp"
#include "udp_server.h"
#include "ws_server.h"

extern App g_app;

void Engine::Run(tcp::endpoint ws_endpoint,
                 udp::endpoint udp_endpoint,
                 std::string audio_codec,
                 uint64_t audio_bitrate,
                 bool enable_nvenc,
                 KeyboardReplay keyboard_replay,
                 GamepadReplay gamepad_replay,
                 uint64_t video_bitrate,
                 AVCodecID video_codec_id,
                 int video_gop,
                 std::string video_preset,
                 uint32_t video_quality) {
  try {
    if (!audio_encoder_.Init(std::move(audio_codec), audio_bitrate)) {
      std::cout << "Initialize audio encoder failed!\n";
      return;
    }

    if (!video_encoder_.Init(enable_nvenc, video_bitrate, video_codec_id,
                             video_gop, std::move(video_preset),
                             video_quality)) {
      std::cout << "Initialize video encoder failed!\n";
      return;
    }

    if (0 != ws_endpoint.port()) {
      ws_server_ = std::make_shared<WsServer>(*this, ws_endpoint);
      std::cout << "WebSocket server on: " << ws_endpoint << '\n';
      ws_server_->Run();
    }
    udp_server_ = std::make_shared<UdpServer>(*this, udp_endpoint,
                                              std::move(keyboard_replay),
                                              std::move(gamepad_replay));
    std::cout << "UDP Server on: " << udp_endpoint << '\n';
    udp_server_->Run();
  } catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    return;
  }

  running_ = true;
  auto io_thread = std::thread(&Engine::Loop, this);
  Loop();
  io_thread.join();
  running_ = false;
}

void Engine::Loop() noexcept {
  for (;;) {
    try {
      ioc_.run();
      break;
    } catch (std::exception& e) {
#if _DEBUG
      std::cerr << "# " << e.what() << '\n';
#else
      boost::ignore_unused(e);
#endif
    }
  }
}

void Engine::Stop() {
  if (!running_) {
    return;
  }
  ws_server_->Stop(false);
  udp_server_->Stop();
  try {
    ioc_.stop();
    running_ = false;
  } catch (std::exception& e) {
#if _DEBUG
    std::cerr << e.what() << '\n';
#else
    boost::ignore_unused(e);
#endif
  }
}

void Engine::EncoderRun() {
  audio_encoder_.Run();
  video_encoder_.Run();
}

void Engine::EncoderStop() {
  audio_encoder_.Stop();
  video_encoder_.Stop();
}

int Engine::OnWriteHeader(void* opaque,
                          uint8_t* buffer,
                          int buffer_size) noexcept {
#if _DEBUG
  // std::cout << __func__ << ": " << buffer_size << "\n";
#endif
  auto ei = static_cast<Encoder*>(opaque);
  ei->SaveHeader(buffer, buffer_size);
  return 0;
}

int Engine::OnWritePacket(void* opaque,
                          uint8_t* buffer,
                          int buffer_size) noexcept {
#if _DEBUG
  // std::cout << __func__ << ": " << buffer_size << "\n";
#endif
  return Engine::GetInstance().WritePacket(opaque, buffer, buffer_size);
}

int Engine::WritePacket(void* opaque, uint8_t* body, int body_size) noexcept {
  auto ei = static_cast<Encoder*>(opaque);
  std::string buffer;
  buffer.resize(sizeof(regame::NetPacketHeader) + body_size);
  auto header = reinterpret_cast<regame::NetPacketHeader*>(buffer.data());
  header->version = regame::kNetPacketCurrentVersion;
  header->type = ei->GetType();
  header->size = htonl(body_size);
  memcpy(buffer.data() + sizeof(regame::NetPacketHeader), body, body_size);
  ws_server_->Send(std::move(buffer));
  return 0;
}

void Engine::SetPresentFlag(bool donot_present) {
  HANDLE ev =
      CreateEvent(g_app.SA(), TRUE, FALSE, kDoNotPresentEventName.data());
  if (nullptr == ev) {
    std::cerr << "CreateEvent() failed with " << GetLastError() << '\n';
    return;
  }
  donot_present_event_.Attach(ev);
  if (donot_present) {
    SetEvent(donot_present_event_);
  } else {
    ResetEvent(donot_present_event_);
  }
}
