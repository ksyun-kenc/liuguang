#pragma once

#include "net.hpp"

#include "encoder.h"

enum class KeyboardReplay { NONE = 0, CGVHID };

enum class GamepadReplay { NONE = 0, CGVHID, VIGEM };

struct NetPacketHeader {
  uint32_t type : 8;
  uint32_t ts : 24;
  uint32_t size;
  float elapsed;
};

class Engine {
 public:
  ~Engine() = default;

  static Engine& GetInstance() noexcept {
    static Engine instance;
    return instance;
  }

  static int OnWritePacket(void* opaque,
                           uint8_t* buffer,
                           int buffer_size) noexcept;

  net::io_context& GetIoContext() { return ioc_; }

  void Run(tcp::endpoint ws_endpoint,
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
           uint32_t video_quality);
  void Stop();

  void EncoderRun();
  void EncoderStop();

  void SetPresentFlag(bool donot_present);

 private:
  Engine() = default;

  int WritePacket(void* opaque,
                  uint32_t timestamp,
                  uint8_t* buffer,
                  int buffer_size) noexcept;

  void Loop() noexcept;

 private:
  bool running_ = false;

  net::io_context ioc_{2};
  std::shared_ptr<class WsServer> ws_server_;
  std::shared_ptr<class UdpServer> udp_server_;
  Encoder encoder_;

  CHandle donot_present_event_;
};
