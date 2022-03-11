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

#include "encoder.h"
#include "sound_capturer.h"

#include "regame/shared_mem_info.h"

class ObjectNamer;

class AudioEncoder : public Encoder {
 public:
  AudioEncoder(ObjectNamer& object_namer)
      : Encoder(regame::ServerAction::kAudio), object_namer_(object_namer) {}
  ~AudioEncoder() = default;

  bool Initialize(std::string codec_name, uint64_t bitrate) noexcept;
  int Run();
  void Stop();

 private:
  int EncodingThread();
  void Free(bool wait_thread);
  int AddStream(const AVCodec*& codec);
  int Open(const AVCodec* codec, AVDictionary** opts);
  int InitializeFrame(AVFrame*& frame) const noexcept;
  int Encode();

 private:
  ObjectNamer& object_namer_;
  std::string codec_name_;
  uint64_t bitrate_;

  CHandle started_event_;
  CHandle stop_event_;
  std::thread thread_;

  SoundCapturer sound_capturer_{object_namer_};
  AudioInfo source_audio_info_;

  CAtlFileMapping<regame::SharedAudioFrames> shared_frames_;
  CHandle shared_frame_ready_event_;

  AVStream* stream_ = nullptr;
  AVCodecContext* codec_context_ = nullptr;
  AVFormatContext* format_context_ = nullptr;
  int64_t next_pts_ = 0;
};
