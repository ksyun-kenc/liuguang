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

#include <sddl.h>

#include <thread>

#include "../EasyHook/Public/easyhook.h"

#include "hook_d3d9.h"
#include "hook_dxgi.h"

#include "regame/shared_mem_info.h"

class CaptureYuv {
 public:
  static CaptureYuv& GetInstance() noexcept {
    static CaptureYuv instance;
    return instance;
  }

  bool Run() noexcept;

  void Free() noexcept;

  bool AttemptToHook() noexcept;

  const SECURITY_ATTRIBUTES* GetSA() const noexcept {
    assert(nullptr != sa_.lpSecurityDescriptor);
    return &sa_;
  }
  SECURITY_ATTRIBUTES* SA() noexcept {
    assert(nullptr != sa_.lpSecurityDescriptor);
    return &sa_;
  }

  HANDLE GetStopEvent() const noexcept {
    assert(nullptr != stop_event_);
    return stop_event_;
  }

  bool IsEncoderStarted() const noexcept { return is_encoder_started_; }
  bool IsPresentEnabled() const noexcept { return is_present_enabled_; }

  bool CreateSharedFrameInfo() noexcept {
    if (nullptr == shared_frame_info_) {
      HRESULT hr = shared_frame_info_.MapSharedMem(
          sizeof(regame::SharedVideoFrameInfo),
          regame::kSharedVideoFrameInfoFileMappingName.data(), nullptr, SA());
      if (FAILED(hr)) {
        ATLTRACE2(atlTraceException, 0,
                  "MapSharedMem(info) failed with 0x%08x.\n", hr);
        return false;
      }
    }
    return true;
  }
  regame::SharedVideoFrameInfo* GetSharedVideoFrameInfo() const noexcept {
    assert(nullptr != shared_frame_info_);
    return shared_frame_info_;
  }
  void FreeSharedVideoFrameInfo() noexcept { shared_frame_info_.Unmap(); }

  bool CreateSharedVideoYuvFrames(size_t data_size) noexcept;
  regame::PackedVideoYuvFrame* GetPackedVideoYuvFrame(
      size_t index) const noexcept {
    assert(index < regame::kNumberOfSharedFrames);

    auto shared_frame =
        static_cast<regame::SharedVideoYuvFrames*>(shared_frames_.GetData());
    return reinterpret_cast<regame::PackedVideoYuvFrame*>(
        static_cast<char*>(shared_frames_) +
        sizeof(regame::SharedVideoYuvFrames) + index * shared_frame->data_size);
  }
  regame::PackedVideoYuvFrame* GetAvailablePackedVideoYuvFrame()
      const noexcept {
    return GetPackedVideoYuvFrame(frame_count_ % regame::kNumberOfSharedFrames);
  }
  size_t GetFrameCount() const noexcept { return frame_count_; }
  void SetSharedFrameReadyEvent() noexcept {
    ++frame_count_;
    SetEvent(shared_frame_ready_event_);
  }
  void FreeSharedVideoYuvFrames() noexcept {
    shared_frames_.Unmap();
    shared_frame_ready_event_.Close();
  }

 private:
  bool Initialize() noexcept;
  int HookThread() noexcept;

 private:
  SECURITY_ATTRIBUTES sa_{};
  CHandle stop_event_;
  std::thread hook_thread_;

  CAtlFileMapping<regame::SharedVideoFrameInfo> shared_frame_info_;
  CAtlFileMapping<char> shared_frames_;
  CHandle shared_frame_ready_event_;
  size_t frame_count_{0};

  CHandle encoder_started_event_;
  bool is_encoder_started_{false};

  bool is_present_enabled_{true};

  bool is_d3d9_hooked_{false};
  HookD3d9 hook_d3d9_;

  bool is_dxgi_hooked_{false};
  HookDxgi hook_dxgi_;
};
