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

#include <atlfile.h>

#include <chrono>

#include "encoder.h"

#include "regame/shared_mem_info.h"

enum class HardwareEncoder { None = 0, AMF, NVENC, QSV };

class VideoEncoder : public Encoder {
 public:
  VideoEncoder() noexcept : Encoder(regame::ServerAction::kVideo) {}
  ~VideoEncoder() noexcept = default;

  bool Init(uint64_t bitrate,
            AVCodecID codec_id,
            HardwareEncoder hardware_encoder,
            int gop,
            std::string video_preset,
            uint32_t quality) noexcept;
  int Run();
  void Stop();

  HWND GetSourceWindow() const noexcept {
    return reinterpret_cast<HWND>(saved_frame_info_.window);
  }

 private:
  int EncodingThread();
  void Free(bool wait_thread);
  int AddStream(const AVCodec*& codec);
  int Open(const AVCodec* codec, AVDictionary** opts);
  int InitFrame(AVFrame*& frame) const noexcept;
  int EncodeFrame(AVFrame* frame) noexcept;
  int EncodeYuvFrame(AVFrame* frame, const uint8_t* yuv) noexcept;

 private:
  HardwareEncoder hardware_encoder_;
  uint64_t bitrate_;
  int gop_;
  std::string video_preset_;
  uint32_t quality_;

  CHandle started_event_;
  CHandle stop_event_;
  std::thread thread_;

  CAtlFileMapping<SharedVideoFrameInfo> shared_frame_info_;
  SharedVideoFrameInfo saved_frame_info_;
  CAtlFileMapping<uint8_t> shared_frames_;
  CHandle shared_frame_ready_event_;

  AVStream* stream_ = nullptr;
  AVCodecContext* codec_context_ = nullptr;
  AVFormatContext* format_context_ = nullptr;

  std::chrono::steady_clock::time_point startup_time_;
};
