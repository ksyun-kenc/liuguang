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

#include <d3d11_2.h>
#include <dxgi.h>

#include <SDL2/SDL.h>

#include <thread>

#include "regame/shared_mem_info.h"

struct PooledSurface {
  VideoFrameStats stats;
  IDXGISurface* surface;
};

class SdlHack {
 public:
  SdlHack() = default;
  ~SdlHack();

  bool Init() noexcept;
  bool IsStarted() const noexcept;
  void GetTexture(SDL_Renderer* renderer);

 private:
  int WaitingThread();

 private:
  SECURITY_ATTRIBUTES sa_{};
  IDXGISwapChain1* swap_{nullptr};
  std::atomic<bool> is_started_{false};
  std::thread waiting_thread_;
  size_t frame_count_{0};
  CHandle encoder_started_event_;
  CHandle encoder_stopped_event_;
  CHandle stop_event_;
  CAtlFileMapping<SharedVideoFrameInfo> shared_frame_info_;
  CAtlFileMapping<char> shared_frames_;
  CHandle shared_frame_ready_event_;
  bool matched_version_ = false;
};
