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

void Engine::Run(tcp::endpoint ws_endpoint,
                 std::string audio_codec,
                 uint64_t audio_bitrate,
                 const std::vector<uint8_t>& disable_keys,
                 GamepadReplay gamepad_replay,
                 KeyboardReplay keyboard_replay,
                 MouseReplay mouse_replay,
                 uint64_t video_bitrate,
                 AVCodecID video_codec_id,
                 HardwareEncoder hardware_encoder,
                 int video_gop,
                 std::string video_preset,
                 uint32_t video_quality) {
  try {
    if (!audio_encoder_.Init(std::move(audio_codec), audio_bitrate)) {
      APP_ERROR() << "Initialize audio encoder failed!\n";
      return;
    }

    if (!video_encoder_.Init(video_bitrate, video_codec_id, hardware_encoder,
                             video_gop, std::move(video_preset),
                             video_quality)) {
      APP_ERROR() << "Initialize video encoder failed!\n";
      return;
    }

    game_service_ =
        std::make_shared<GameService>(ws_endpoint, disable_keys, gamepad_replay,
                                      keyboard_replay, mouse_replay);
    APP_INFO() << "Regame service via WebSocket on " << ws_endpoint << '\n';
    game_service_->Run();
  } catch (std::exception& e) {
    APP_FATAL() << e.what() << '\n';
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
      APP_ERROR() << e.what() << '\n';
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
  game_service_->Stop(false);
  try {
    ioc_.stop();
    running_ = false;
  } catch (std::exception& e) {
#if _DEBUG
    APP_ERROR() << e.what() << '\n';
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

int Engine::OnWriteHeader(void* opaque, uint8_t* data, int size) noexcept {
  auto ei = static_cast<Encoder*>(opaque);
  ei->SaveHeader(std::span(data, size));
  return 0;
}

int Engine::OnWritePacket(void* opaque, uint8_t* data, int size) noexcept {
  return g_app.GetEngine().WritePacket(opaque, std::span(data, size));
}

int Engine::WritePacket(void* opaque, std::span<uint8_t> packet) noexcept {
  auto ei = static_cast<Encoder*>(opaque);
  std::string buffer;
  buffer.resize(sizeof(regame::PackageHead) + sizeof(regame::ServerPacket) +
                packet.size());
  auto head = reinterpret_cast<regame::PackageHead*>(buffer.data());
  head->size =
      htonl(static_cast<int>(sizeof(regame::ServerPacket) + packet.size()));
  auto server_packet = reinterpret_cast<regame::ServerPacket*>(head + 1);
  server_packet->action = ei->GetServerAction();
  memcpy(server_packet + 1, packet.data(), packet.size());
  game_service_->Send(std::move(buffer));
  return 0;
}

void Engine::SetPresentFlag(bool donot_present) {
  HANDLE ev =
      CreateEvent(g_app.SA(), TRUE, FALSE, kDoNotPresentEventName.data());
  if (nullptr == ev) {
    APP_ERROR() << "CreateEvent() failed with " << GetLastError() << '\n';
    return;
  }
  donot_present_event_.Attach(ev);
  if (donot_present) {
    SetEvent(donot_present_event_);
  } else {
    ResetEvent(donot_present_event_);
  }
}

void Engine::NotifyRestartAudioEncoder() noexcept {}

void Engine::NotifyRestartVideoEncoder() noexcept {
  if (game_service_) {
    std::string buffer;
    buffer.resize(sizeof(regame::PackageHead) + sizeof(regame::ServerPacket));
    auto head = reinterpret_cast<regame::PackageHead*>(buffer.data());
    head->size = htonl(sizeof(regame::ServerPacket));
    auto packet = reinterpret_cast<regame::ServerPacket*>(head + 1);
    packet->action = regame::ServerAction::kResetVideo;
    game_service_->Send(std::move(buffer));
  }

  net::dispatch(ioc_, std::bind(&Engine::RestartVideoEncoder, this));
}

void Engine::RestartVideoEncoder() noexcept {
  video_encoder_.Stop();
  video_encoder_.Run();
}