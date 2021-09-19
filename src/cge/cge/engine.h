#pragma once

#include "net.hpp"

#include "audio_encoder.h"
#include "game_service.h"
#include "video_encoder.h"

class Engine {
 public:
  ~Engine() = default;

  static Engine& GetInstance() noexcept {
    static Engine instance;
    return instance;
  }

  static int OnWriteHeader(void* opaque, uint8_t* data, int size) noexcept;
  static int OnWritePacket(void* opaque, uint8_t* data, int size) noexcept;

  net::io_context& GetIoContext() { return ioc_; }

  void Run(tcp::endpoint ws_endpoint,
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
           uint32_t video_quality);
  void Stop();

  void EncoderRun();
  void EncoderStop();

  void SetPresentFlag(bool donot_present);

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

 private:
  Engine() = default;

  int WritePacket(void* opaque, std::span<uint8_t> packet) noexcept;

  void Loop() noexcept;

  void RestartVideoEncoder() noexcept;

 private:
  bool running_ = false;

  net::io_context ioc_{2};
  std::shared_ptr<GameService> game_service_;

  AudioEncoder audio_encoder_;
  VideoEncoder video_encoder_;

  CHandle donot_present_event_;
};
