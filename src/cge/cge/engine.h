#pragma once

#include "net.hpp"

#include "audio_encoder.h"
#include "video_encoder.h"

enum class KeyboardReplay { NONE = 0, CGVHID };

enum class GamepadReplay { NONE = 0, CGVHID, VIGEM };

class Engine {
 public:
  ~Engine() = default;

  static Engine& GetInstance() noexcept {
    static Engine instance;
    return instance;
  }

  static int OnWriteHeader(void* opaque,
                           uint8_t* buffer,
                           int buffer_size) noexcept;
  static int OnWritePacket(void* opaque,
                           uint8_t* buffer,
                           int buffer_size) noexcept;

  net::io_context& GetIoContext() { return ioc_; }

  void Run(tcp::endpoint ws_endpoint,
           udp::endpoint udp_endpoint,
           std::string audio_codec,
           uint64_t audio_bitrate,
           KeyboardReplay keyboard_replay,
           GamepadReplay gamepad_replay,
           uint64_t video_bitrate,
           AVCodecID video_codec_id,
           VideoEncoderType video_encoder_type,
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

 private:
  Engine() = default;

  int WritePacket(void* opaque, uint8_t* buffer, int buffer_size) noexcept;

  void Loop() noexcept;

 private:
  bool running_ = false;

  net::io_context ioc_{2};
  std::shared_ptr<class WsServer> ws_server_;
  std::shared_ptr<class UdpServer> udp_server_;

  AudioEncoder audio_encoder_;
  VideoEncoder video_encoder_;

  CHandle donot_present_event_;
};
