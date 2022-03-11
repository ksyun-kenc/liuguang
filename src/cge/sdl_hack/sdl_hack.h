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

#include <array>
#include <atomic>
#include <thread>

#include "regame/sdl_internal.h"
#include "regame/shared_mem_info.h"

struct PooledSurface {
  regame::VideoFrameStats stats;
  IDXGISurface* surface;
};

struct SharedTexture {
  CComPtr<ID3D11Texture2D> texture;
  CComPtr<IDXGIResource1> resource;
  HANDLE handle;
};

class SdlHack {
 public:
  SdlHack() = default;
  ~SdlHack() { Free(); };

  bool Initialize() noexcept;
  bool Run(bool global_mode, regame::VideoFrameType frame_type) noexcept;
  void Free() noexcept;
  bool IsStarted() const noexcept;
  void CopyTexture(SDL_Renderer* renderer);
  std::wstring GetName(std::wstring_view name) noexcept;

 private:
  int WaitingThread();
  void CopyTextureToSharedTexture(D3D11_RenderData* render_data);
  void CopyTextureToSharedYuv(D3D11_RenderData* render_data);

 private:
  SECURITY_ATTRIBUTES sa_{};
  IDXGISwapChain1* swap_{nullptr};
  std::atomic<bool> is_started_{false};
  std::thread waiting_thread_;
  size_t frame_count_{0};
  CHandle encoder_started_event_;
  CHandle encoder_stopped_event_;
  CHandle stop_event_;
  CAtlFileMapping<regame::SharedVideoFrameInfo> shared_frame_info_;
  CAtlFileMapping<char> shared_yuv_frames_;
  CAtlFileMapping<regame::SharedVideoTextureFrames> shared_texture_frames_;
  std::array<SharedTexture, regame::kNumberOfSharedFrames> shared_textures_;
  std::uint64_t texture_id_{0};

  CHandle shared_frame_ready_event_;
  SDL_version linked_;
  UINT width_ = 0;
  UINT height_ = 0;
  bool global_mode_{false};
  regame::VideoFrameType frame_type_{regame::VideoFrameType::kNone};
};
