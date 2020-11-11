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

#include <boost/asio/detail/winsock_init.hpp>
#include <boost/program_options.hpp>

#include "app.hpp"
#include "engine.h"
#include "sound_capturer.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "avrt.lib")

//#ifdef _DEBUG
//#pragma comment(lib, "avutild.lib")
//#pragma comment(lib, "avcodecd.lib")
//#pragma comment(lib, "avformatd.lib")
//#pragma comment(lib, "swresampled.lib")
//#else
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swresample.lib")
//#endif  // DEBUG

#pragma warning(push)
#pragma warning(disable : 4073)
#pragma init_seg(lib)
boost::asio::detail::winsock_init<>::manual g_manual_winsock_init;
#pragma warning(pop)

namespace po = boost::program_options;

constexpr std::string_view kProgramInfo{
    "KSYUN Edge Cloud Gaming Engine v1.0 Beta"};
constexpr std::string_view kDefaultBindAddress{"::"};
constexpr uint64_t kDefaultAudioBitrate = 128000;
constexpr std::string_view kDefaultAudioCodec{"libopus"};
constexpr uint16_t kDefaultControlPort = 8080;
constexpr uint16_t kDefaultStreamPort = 8080;
constexpr uint64_t kDefaultVideoBitrate = 1'000'000;
constexpr int kDefaultVideoGop = 180;
constexpr uint32_t kDefaultVideoQuality = 23;

constexpr uint32_t kMinAudioBitrate = 16'000;
constexpr uint32_t kMaxAudioBitrate = 256'000;
constexpr uint32_t kMinVideoBitrate = 100'000;
constexpr int kMinGop = 1;
constexpr int kMaxGop = 500;
constexpr uint32_t kMaxVideoQuality = 51;

App g_app;

int main(int argc, char* argv[]) {
  std::string audio_codec;
  uint64_t audio_bitrate = 0;
  std::string bind_address;
  uint16_t control_port = 0;
  bool enable_nvenc = true;
  uint16_t stream_port = 0;
  uint64_t video_bitrate = 0;
  int video_gop = 0;
  std::string video_preset;
  uint32_t video_quality = 0;

  try {
    po::options_description desc("Usage");
    // clang-format off
    desc.add_options()("help,h", "produce help message")
      ("audio-bitrate",
        po::value<uint64_t>(&audio_bitrate)->default_value(kDefaultAudioBitrate),
        "set audio bitrate")
      ("audio-codec",
        po::value<std::string>(&audio_codec)->default_value(kDefaultAudioCodec.data()),
        "set audio codec, can be one of {libopus, opus, aac}")
      ("bind-address",
        po::value<std::string>(&bind_address)->default_value(kDefaultBindAddress.data()),
        "set bind address for listening, eg: 0.0.0.0")
      ("control-port",
        po::value<uint16_t>(&control_port)->default_value(kDefaultControlPort),
        "set the UDP port for control flow")
      ("enable-nvenc",
        po::value<bool>(&enable_nvenc)->default_value(true),
        "Enable nvenc")
      ("stream-port",
        po::value<uint16_t>(&stream_port)->default_value(kDefaultStreamPort),
        "set the websocket port for streaming, if port is 0, disable stream "
        "out via network. Capture and encode picture directly at startup but "
        "not on connection establishing, and never stop this until cge exit. "
        "stream port is not same as control port, this port is only for media "
        "output.")
      ("video-bitrate",
        po::value<uint64_t>(&video_bitrate)->default_value(kDefaultVideoBitrate),
        "set video bitrate")
      ("video-gop",
        po::value<int>(&video_gop)->default_value(kDefaultVideoGop),
        "set video gop")
      ("video-preset",
        po::value<std::string>(&video_preset))
      ("video-quality",
        po::value<uint32_t>(&video_quality)->default_value(kDefaultVideoQuality),
        "set video quality, lower is better, available range is 0-51, 0 is "
        "lossless");
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << kProgramInfo << "\n\n" << desc;
      return EXIT_SUCCESS;
    }

    // sanity check
    if (audio_bitrate < kMinAudioBitrate || audio_bitrate > kMaxAudioBitrate) {
      throw std::out_of_range("audio-bitrate out of range!");
    }
    if (audio_codec != kDefaultAudioCodec && audio_codec != "aac" &&
        audio_codec != "opus") {
      throw std::invalid_argument("unsupported audio-codec!");
    }
    if (0 == control_port) {
      control_port = kDefaultControlPort;
    }
    if (video_bitrate < kMinVideoBitrate) {
      throw std::out_of_range("video-bitrate too low!");
    }
    if (video_gop < kMinGop || video_gop > kMaxGop) {
      throw std::out_of_range("video-gop out of range!");
    }
    if (video_preset.empty()) {
      video_preset = enable_nvenc ? "llhp" : "ultrafast";
    }
    if (video_quality > kMaxVideoQuality) {
      throw std::out_of_range("video-quality out of range!");
    }

#if _DEBUG
    std::cout << "audio-bitrate: " << audio_bitrate << '\n'
              << "audio-codec: " << audio_codec << '\n'
              << "bind-address: " << bind_address << '\n'
              << "control-port: " << control_port << '\n'
              << "enable-nvenc: " << std::boolalpha << enable_nvenc << '\n'
              << "stream-port: " << stream_port << '\n'
              << "video-bitrate: " << video_bitrate << '\n'
              << "video-gop: " << video_gop << '\n'
              << "video-preset: " << video_preset << '\n'
              << "video-quality: " << video_quality << '\n';
#endif
  } catch (std::exception& e) {
    std::cerr << "Invalid argument: " << e.what() << "\n";
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Invalid argument: unknown exception!\n";
    return EXIT_FAILURE;
  }

  boost::asio::detail::winsock_init<2, 2> winsock;
  boost::system::error_code ec;
  const auto kAddress = net::ip::make_address(bind_address, ec);
  if (ec) {
    std::cerr << "Invalid bind-address: " << ec.message() << "\n";
    return EXIT_FAILURE;
  }

  if (!g_app.Init()) {
    std::cerr << "Init Engine failed!\n";
    return EXIT_FAILURE;
  }
  net::signal_set signals(Engine::GetInstance().GetIoContext(), SIGINT, SIGTERM,
                          SIGBREAK);
  signals.async_wait([&](const boost::system::error_code&, int sig) {
    std::cout << "receive signal(" << sig << ").\n";
    Engine::GetInstance().Stop();
  });
  Engine::GetInstance().Run(tcp::endpoint(kAddress, stream_port),
                            udp::endpoint(kAddress, control_port),
                            std::move(audio_codec), audio_bitrate, enable_nvenc,
                            video_bitrate, video_gop, std::move(video_preset),
                            video_quality);
  Engine::GetInstance().EncoderStop();
  return EXIT_SUCCESS;
}
