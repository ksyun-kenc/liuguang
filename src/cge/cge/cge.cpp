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

#include "umu/string.h"

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

using namespace std::literals::string_view_literals;

constexpr auto kProgramInfo{"KSYUN Edge Cloud Gaming Engine v0.2 Beta"sv};
constexpr auto kDefaultBindAddress{"::"sv};
constexpr uint64_t kDefaultAudioBitrate = 128000;
constexpr auto kDefaultAudioCodec{"libopus"sv};
constexpr std::array<std::string_view, 3> kValidAudioCodecs = {
    kDefaultAudioCodec, "aac", "opus"};
constexpr uint16_t kDefaultControlPort = 8080;
constexpr bool kDefaultDonotPresent = false;

constexpr auto kDefaultKeyboardReplay{"none"sv};
// Should be the same order with KeyboardReplay
constexpr std::array<std::string_view, 2> kValidKeyboardReplayMethods = {
    kDefaultKeyboardReplay, "cgvhid"};

constexpr auto kDefaultGamepadReplay{"none"sv};
// Should be the same order with GamepadReplay
constexpr std::array<std::string_view, 3> kValidGamepadReplayMethods = {
    kDefaultGamepadReplay, "cgvhid", "vigem"};

constexpr uint16_t kDefaultStreamPort = 8080;
constexpr uint64_t kDefaultVideoBitrate = 1'000'000;
constexpr auto kDefaultVideoCodec{"h264"sv};
constexpr int kDefaultVideoGop = 180;
constexpr std::array<std::string_view, 19> kValidNvencPreset = {
    "default", "slow", "medium", "fast",     "hp",         "hq", "bd",
    "ll",      "llhq", "llhp",   "lossless", "losslesshp", "p1", "p2",
    "p3",      "p4",   "p5",     "p6",       "p7"};
constexpr std::array<std::string_view, 10> kValidPreset = {
    "ultrafast", "superfast", "veryfast", "faster",   "fast",
    "medium",    "slow",      "slower",   "veryslow", "placebo"};
constexpr uint32_t kDefaultVideoQuality = 23;

constexpr uint32_t kMinAudioBitrate = 16'000;
constexpr uint32_t kMaxAudioBitrate = 256'000;
constexpr uint32_t kMinVideoBitrate = 100'000;
constexpr int kMinGop = 1;
constexpr int kMaxGop = 500;
constexpr uint32_t kMaxVideoQuality = 51;

App g_app;

namespace {
const auto io_sync = []() {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  return false;
}();
}

int main(int argc, char* argv[]) {
  std::string audio_codec;
  uint64_t audio_bitrate = 0;
  std::string bind_address;
  uint16_t control_port = 0;
  bool donot_present = false;
  bool enable_nvenc = true;
  KeyboardReplay keyboard_replay;
  GamepadReplay gamepad_replay;
  uint16_t stream_port = 0;
  uint64_t video_bitrate = 0;
  AVCodecID video_codec_id = AV_CODEC_ID_NONE;
  int video_gop = 0;
  std::string video_preset;
  uint32_t video_quality = 0;

  try {
    std::string keyboard_replay_string;
    std::string gamepad_replay_string;
    std::string video_codec;

    po::options_description desc("Usage");
    // clang-format off
    desc.add_options()("help,h", "Produce help message")
      ("audio-bitrate",
        po::value<uint64_t>(&audio_bitrate)->default_value(kDefaultAudioBitrate),
        "Set audio bitrate")
      ("audio-codec",
        po::value<std::string>(&audio_codec)->default_value(kDefaultAudioCodec.data()),
        std::string("Set audio codec. Select one of ")
       .append(umu::string::ArrayJoin(kValidAudioCodecs)).data())
      ("bind-address",
        po::value<std::string>(&bind_address)->default_value(kDefaultBindAddress.data()),
        "Set bind address for listening. eg: 0.0.0.0")
      ("control-port",
        po::value<uint16_t>(&control_port)->default_value(kDefaultControlPort),
        "Set the UDP port for control flow")
      ("donot-present",
        po::value<bool>(&donot_present)->default_value(kDefaultDonotPresent),
        "Tell cgh don't present")
      ("enable-nvenc",
        po::value<bool>(&enable_nvenc)->default_value(true),
        "Enable nvenc")
      ("keyboard-replay",
        po::value<std::string>(&keyboard_replay_string)->default_value(kDefaultKeyboardReplay.data()),
        std::string("Set keyboard replay method. Select one of ")
        .append(umu::string::ArrayJoin(kValidKeyboardReplayMethods)).data())
      ("gamepad-replay",
        po::value<std::string>(&gamepad_replay_string)->default_value(kDefaultGamepadReplay.data()),
        std::string("Set gamepad replay method. Select one of ")
       .append(umu::string::ArrayJoin(kValidGamepadReplayMethods)).data())
      ("stream-port",
        po::value<uint16_t>(&stream_port)->default_value(kDefaultStreamPort),
        "Set the websocket port for streaming, if port is 0, disable stream "
        "out via network. Capture and encode picture directly at startup but "
        "not on connection establishing, and never stop this until cge exit. "
        "stream port is not same as control port, this port is only for media "
        "output.")
      ("video-bitrate",
        po::value<uint64_t>(&video_bitrate)->default_value(kDefaultVideoBitrate),
        "Set video bitrate")
      ("video-codec",
        po::value<std::string>(&video_codec)->default_value(kDefaultVideoCodec.data()),
        "Set video codec. Select one of {h264, h265, hevc}, h265 == hevc")
      ("video-gop",
        po::value<int>(&video_gop)->default_value(kDefaultVideoGop),
        "Set video gop. [1, 500]")
      ("video-preset",
        po::value<std::string>(&video_preset),
        std::string("Set preset for video encoder. When enable nvenc, select one of ")
       .append(umu::string::ArrayJoin(kValidNvencPreset))
       .append("; otherwise, select one of ")
       .append(umu::string::ArrayJoin(kValidPreset)).data())
      ("video-quality",
        po::value<uint32_t>(&video_quality)->default_value(kDefaultVideoQuality),
        "Set video quality. [0, 51], lower is better, 0 is lossless");
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
    if (kValidAudioCodecs.cend() == std::find(kValidAudioCodecs.cbegin(),
                                              kValidAudioCodecs.cend(),
                                              audio_codec)) {
      throw std::invalid_argument("unsupported audio-codec!");
    }
    if (0 == control_port) {
      control_port = kDefaultControlPort;
    }

    auto keyboard_replay_pos =
        std::find(kValidKeyboardReplayMethods.cbegin(),
                  kValidKeyboardReplayMethods.cend(), keyboard_replay_string);
    if (kValidKeyboardReplayMethods.cend() == keyboard_replay_pos) {
      throw std::invalid_argument("unsupported keyboard-replay!");
    }
    keyboard_replay = static_cast<KeyboardReplay>(std::distance(
        kValidKeyboardReplayMethods.cbegin(), keyboard_replay_pos));

    auto gamepad_replay_pos =
        std::find(kValidGamepadReplayMethods.cbegin(),
                  kValidGamepadReplayMethods.cend(), gamepad_replay_string);
    if (kValidGamepadReplayMethods.cend() == gamepad_replay_pos) {
      throw std::invalid_argument("unsupported gamepad-replay!");
    }
    gamepad_replay = static_cast<GamepadReplay>(
        std::distance(kValidGamepadReplayMethods.cbegin(), gamepad_replay_pos));

    if (video_bitrate < kMinVideoBitrate) {
      throw std::out_of_range("video-bitrate too low!");
    }
    if (video_codec == "h264") {
      video_codec_id = AV_CODEC_ID_H264;
    } else if (video_codec == "h265" || video_codec == "hevc") {
      video_codec_id = AV_CODEC_ID_HEVC;
    } else {
      throw std::invalid_argument("unsupported video-codec!");
    }
    if (video_gop < kMinGop || video_gop > kMaxGop) {
      throw std::out_of_range("video-gop out of range!");
    }
    if (video_preset.empty()) {
      video_preset = enable_nvenc ? "llhp" : "ultrafast";
    } else {
      if (enable_nvenc) {
        if (kValidNvencPreset.cend() == std::find(kValidNvencPreset.cbegin(),
                                                  kValidNvencPreset.cend(),
                                                  video_preset)) {
          throw std::invalid_argument("unsupported NvEnc video-preset!");
        }
      } else {
        if (kValidPreset.cend() == std::find(kValidPreset.cbegin(),
                                             kValidPreset.cend(),
                                             video_preset)) {
          throw std::invalid_argument("unsupported NvEnc video-preset!");
        }
      }
    }
    if (video_quality > kMaxVideoQuality) {
      throw std::out_of_range("video-quality out of range!");
    }

#if _DEBUG
    std::cout << "audio-bitrate: " << audio_bitrate << '\n'
              << "audio-codec: " << audio_codec << '\n'
              << "bind-address: " << bind_address << '\n'
              << "control-port: " << control_port << '\n'
              << "donot-present: " << std::boolalpha << donot_present << '\n'
              << "enable-nvenc: " << std::boolalpha << enable_nvenc << '\n'
              << "keyboard-replay: " << keyboard_replay_string << '\n'
              << "gamepad-replay: " << gamepad_replay_string << '\n'
              << "stream-port: " << stream_port << '\n'
              << "video-bitrate: " << video_bitrate << '\n'
              << "video-codec: " << video_codec << '\n'
              << "video-gop: " << video_gop << '\n'
              << "video-preset: " << video_preset << '\n'
              << "video-quality: " << video_quality << '\n';
#endif
  } catch (const std::invalid_argument& e) {
    std::cerr << "Invalid argument: " << e.what() << '\n';
    return EXIT_FAILURE;
  } catch (const std::out_of_range& e) {
    std::cerr << "Out of range: " << e.what() << '\n';
    return EXIT_FAILURE;
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Error: unknown exception!\n";
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

  Engine::GetInstance().SetPresentFlag(donot_present);

  net::signal_set signals(Engine::GetInstance().GetIoContext(), SIGINT, SIGTERM,
                          SIGBREAK);
  signals.async_wait([&](const boost::system::error_code&, int sig) {
    std::cout << "receive signal(" << sig << ").\n";
    Engine::GetInstance().Stop();
  });
  Engine::GetInstance().Run(tcp::endpoint(kAddress, stream_port),
                            udp::endpoint(kAddress, control_port),
                            std::move(audio_codec), audio_bitrate, enable_nvenc,
                            keyboard_replay, gamepad_replay, video_bitrate,
                            video_codec_id, video_gop, std::move(video_preset),
                            video_quality);
  Engine::GetInstance().EncoderStop();
  return EXIT_SUCCESS;
}
