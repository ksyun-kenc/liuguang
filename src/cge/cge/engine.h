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

#include "net.hpp"

#include "audio_encoder.h"
#include "game_service.h"
#include "object_namer.h"
#include "video_encoder.h"

class Engine {
 public:
  Engine() = default;
  ~Engine() = default;

  static int OnWriteHeader(void* opaque, uint8_t* data, int size) noexcept;
  static int OnWritePacket(void* opaque, uint8_t* data, int size) noexcept;

  net::io_context& GetIoContext() { return ioc_; }

  void Run(tcp::endpoint ws_endpoint,
           std::string audio_codec,
           uint64_t audio_bitrate,
           const std::vector<uint8_t>& disable_keys,
           GamepadReplay gamepad_replay,
           bool is_desktop_mode,
           KeyboardReplay keyboard_replay,
           MouseReplay mouse_replay,
           uint64_t video_bitrate,
           AVCodecID video_codec_id,
           HardwareEncoder hardware_encoder,
           int video_gop,
           std::string video_preset,
           uint32_t video_quality,
           const std::string& user_service) noexcept;
  void Stop() noexcept;

  void EncoderRun();
  void EncoderStop();

  void DisablePresent(bool donot_present);

  const std::string& GetAudioHeader() const noexcept {
    return audio_encoder_.GetHeader();
  }
  const std::string& GetVideoHeader() const noexcept {
    return video_encoder_.GetHeader();
  }

  AVCodecID GetAudioCodecID() const noexcept {
    return audio_encoder_.GetCodecID();
  }
  AVCodecID GetVideoCodecID() const noexcept {
    return video_encoder_.GetCodecID();
  }

  void NotifyRestartAudioEncoder() noexcept;
  void NotifyRestartVideoEncoder() noexcept;

  HWND GetSourceWindow() const noexcept {
    return video_encoder_.GetSourceWindow();
  }
  std::uint32_t GetSourceWidth() const noexcept {
    return video_encoder_.GetSourceWidth();
  }
  std::uint32_t GetSourceHeight() const noexcept {
    return video_encoder_.GetSourceHeight();
  }

  void VideoProduceKeyframe() noexcept { video_encoder_.ProduceKeyframe(); }
  
  const tcp::endpoint& GetUserServiceEndpoint() noexcept {
    return user_service_endpoint_;
  }
  const std::string& GetUserServiceTarget() noexcept {
    return user_service_target_;
  }

  ObjectNamer& GetObjectNamer() noexcept { return object_namer_; }

  const bool IsDesktopMode() const noexcept { return is_desktop_mode_; }

 private:
  int WritePacket(void* opaque, std::span<uint8_t> packet) noexcept;

  void Loop() noexcept;

  void RestartVideoEncoder() noexcept;

 private:
  bool running_ = false;

  net::io_context ioc_{2};
  std::shared_ptr<GameService> game_service_;

  ObjectNamer object_namer_;
  AudioEncoder audio_encoder_{object_namer_};
  VideoEncoder video_encoder_{object_namer_};

  CHandle donot_present_event_;

  tcp::endpoint user_service_endpoint_;
  std::string user_service_target_;

  bool is_desktop_mode_{false};
};
