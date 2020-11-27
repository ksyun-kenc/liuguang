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
#include <wincodec.h>

#include "yuv/yuv.h"

#include "umu/apppath_t.h"
#include "umu/time_measure.hpp"

#include "shared_mem_info.h"

// struct PooledSurface {
//  VideoFrameStats stats;
//  IDirect3DSurface9* surface;
//  IDirect3DSurface9* render_target;
//};

class D3D9Capture {
 public:
  static D3D9Capture& GetInstance() {
    static D3D9Capture instance;
    return instance;
  }

  void Free() {
    Reset();

    FreeSharedMemory();
    shared_frame_info_.Unmap();
    encoder_started_event_.Close();
    donot_present_event_.Close();
    shared_frame_ready_event_.Close();
  }

  void Capture(IDirect3DDevice9* device, IDirect3DSurface9* backbuffer) {
    if (!initialized_ || device != device_) {
      if (initialized_) {
        ATLTRACE2(atlTraceException, 0, __FUNCTION__ ": old = %p, new = %p\n",
                  device_, device);
        Reset();
      }
      initialized_ = Initialize(device);
      if (!initialized_) {
        ATLTRACE2(atlTraceException, 0, __FUNCTION__ ": !Initialize()\n");
        return;
      }
      should_update_ = true;
    }

    if (!is_encoder_started_) {
      return;
    }

    LARGE_INTEGER tick;
    QueryPerformanceCounter(&tick);

    query_->Issue(D3DISSUE_BEGIN);

    HRESULT hr = device_->GetRenderTargetData(backbuffer, copy_surface_);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": !GetRenderTargetData(), #0x%08X\n", hr);
      return;
    }

    query_->Issue(D3DISSUE_END);

    D3DSURFACE_DESC desc = {};
    hr = copy_surface_->GetDesc(&desc);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": %p->GetDesc() failed with 0x%08X.\n",
                copy_surface_, hr);
      return;
    }
    if (desc.Type != D3DRTYPE_SURFACE && desc.Type != D3DRTYPE_TEXTURE) {
      ATLTRACE2(atlTraceException, 0, __FUNCTION__ ": %p->Desc.Type = %d.\n",
                copy_surface_, desc.Type);
      return;
    }
    if (desc.MultiSampleType != D3DMULTISAMPLE_NONE) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": %p->Desc.MultiSampleType = %d.\n",
                copy_surface_, desc.MultiSampleType);
      return;
    }

    if (should_update_) {
      SharedVideoFrameInfo* svfi = shared_frame_info_;
      svfi->timestamp = tick.QuadPart;
      svfi->type = VideoFrameType::YUV;
      svfi->width = desc.Width;
      svfi->height = desc.Height;
      svfi->format = desc.Format;

      should_update_ = false;
    }

    VideoFrameStats stats = {};
    stats.timestamp = tick.QuadPart;
    stats.elapsed.preprocess = umu::TimeMeasure::Delta(tick.QuadPart);
    ATLTRACE2(atlTraceUtil, 0, __FUNCTION__ ": stats.elapsed.prepare = %llu.\n",
              stats.elapsed.preprocess);

    // 单线程处理，wait_rgb_mapping = 0
    D3DLOCKED_RECT mapped_rect = {};
    {
      umu::TimeMeasure tm(stats.elapsed.rgb_mapping);
      // LockRect 偶尔会很耗时！
      // ATLTRACE2(atlTraceUtil, 0, __FUNCTION__ ": LockRect +\n");
      hr = copy_surface_->LockRect(&mapped_rect, nullptr, D3DLOCK_READONLY);
      // ATLTRACE2(atlTraceUtil, 0, __FUNCTION__ ": LockRect -, 0x%08X\n", hr);
    }
    ATLTRACE2(atlTraceUtil, 0,
              __FUNCTION__ ": stats.elapsed.rgb_mapping = %llu.\n",
              stats.elapsed.rgb_mapping);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": %p->LockRect() failed with 0x%08X.\n",
                copy_surface_, hr);
      return;
    }
    BOOST_SCOPE_EXIT_ALL(&) { copy_surface_->UnlockRect(); };

    size_t pixel_size = desc.Width * desc.Height;
    size_t frame_size = mapped_rect.Pitch * desc.Height;
    size_t data_size = sizeof(PackedVideoYuvFrame) + frame_size;

    if (!CreateSharedMemory(data_size)) {
      return;
    }

    auto frame = GetPackedVideoYuvFrame(frame_count_ % kNumberOfSharedFrames);
    const int uv_stride = (desc.Width + 1) / 2;
    uint8_t* y = reinterpret_cast<uint8_t*>(frame->data);
    uint8_t* u = y + pixel_size;
    uint8_t* v = u + (pixel_size / 4);

    {
      umu::TimeMeasure tm(stats.elapsed.yuv_convert);
      if (D3DFMT_X8R8G8B8 == desc.Format) {
        ARGBToI420(static_cast<uint8_t*>(mapped_rect.pBits), mapped_rect.Pitch,
                   y, desc.Width, u, uv_stride, v, uv_stride, desc.Width,
                   desc.Height);
      } else {
        ATLTRACE2(atlTraceUtil, 0, __FUNCTION__ ": Format = %d\n", desc.Format);
      }
    }
    ATLTRACE2(
        atlTraceUtil, 0,
        __FUNCTION__ ": stats.elapsed.yuv_convert = %llu, frame_count: %zd.\n",
        stats.elapsed.yuv_convert, frame_count_);

    stats.elapsed.total = umu::TimeMeasure::Delta(stats.timestamp);

    frame->stats.timestamp = stats.timestamp;
    frame->stats.elapsed.preprocess = stats.elapsed.preprocess;
    frame->stats.elapsed.nvenc = 0;
    frame->stats.elapsed.wait_rgb_mapping = stats.elapsed.wait_rgb_mapping;
    frame->stats.elapsed.rgb_mapping = stats.elapsed.rgb_mapping;
    frame->stats.elapsed.yuv_convert = stats.elapsed.yuv_convert;
    frame->stats.elapsed.total = stats.elapsed.total;

    ++frame_count_;
    SetEvent(shared_frame_ready_event_);
  }

  void Reset() {
    initialized_ = false;
    copy_surface_.Release();
    query_.Release();

    SetEvent(stop_event_);
    if (nullptr != waiting_thread_.native_handle()) {
      if (waiting_thread_.joinable()) {
        waiting_thread_.join();
      }
    }
  }

  inline bool PresentBegin(IDirect3DDevice9* device,
                           IDirect3DSurface9** backbuffer) {
    HRESULT hr = GetBackbuffer(device, backbuffer);
    if (SUCCEEDED(hr)) {
      Capture(device, *backbuffer);
    } else {
      // D3DERR_INVALIDCALL = 0x8876086C
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": !GetBackbuffer(), #0x%08X\n", hr);
    }
    return !donot_present_;
  }

  inline void PresentEnd(IDirect3DDevice9* device,
                         IDirect3DSurface9* backbuffer) {
    if (nullptr != backbuffer) {
      backbuffer->Release();
    }
  }

 private:
  bool Initialize(IDirect3DDevice9* device) {
    device_ = device;

    if (!InitFormatBackbuffer()) {
      return false;
    }

    HRESULT hr = device_->CreateOffscreenPlainSurface(
        cx_, cy_, d3d9_format_, D3DPOOL_SYSTEMMEM, &copy_surface_, nullptr);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": !CreateOffscreenPlainSurface(), #0x%08X\n", hr);
      return false;
    }

    hr = device_->CreateQuery(D3DQUERYTYPE_EVENT, &query_);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": !CreateQuery(), #0x%08X\n", hr);
      return false;
    }

    if (nullptr == encoder_started_event_) {
      HANDLE ev = CreateEvent(HookD3D9::GetInstance().SA(), TRUE, FALSE,
                              kVideoStartedEventName.data());
      if (nullptr == ev) {
        ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
                  GetLastError());
        return false;
      }
      encoder_started_event_.Attach(ev);
    }

    if (nullptr == encoder_stopped_event_) {
      HANDLE ev = CreateEvent(HookD3D9::GetInstance().SA(), TRUE, FALSE,
                              kVideoStoppedEventName.data());
      if (nullptr == ev) {
        ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
                  GetLastError());
        return false;
      }
      encoder_stopped_event_.Attach(ev);
    }

    if (nullptr == donot_present_event_) {
      HANDLE ev = CreateEvent(HookD3D9::GetInstance().SA(), TRUE, FALSE,
                              kDoNotPresentEventName.data());
      if (nullptr == ev) {
        ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
                  GetLastError());
        return false;
      }
      donot_present_event_.Attach(ev);
    }

    if (nullptr == stop_event_) {
      HANDLE ev =
          CreateEvent(HookD3D9::GetInstance().SA(), TRUE, FALSE, nullptr);
      if (nullptr == ev) {
        ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
                  GetLastError());
        return false;
      }
      stop_event_.Attach(ev);
    }

    if (nullptr == waiting_thread_.native_handle()) {
      waiting_thread_ = std::thread(&D3D9Capture::WaitingThread, this);
      if (nullptr == waiting_thread_.native_handle()) {
        ATLTRACE2(atlTraceException, 0, "Create thread failed with %u\n",
                  GetLastError());
        return false;
      }
    }

    if (nullptr == shared_frame_info_) {
      hr = shared_frame_info_.MapSharedMem(
          sizeof(SharedVideoFrameInfo),
          kSharedVideoFrameInfoFileMappingName.data(), nullptr,
          HookD3D9::GetInstance().SA());
      if (FAILED(hr)) {
        ATLTRACE2(atlTraceException, 0, "MapSharedMem() failed with 0x%08x.\n",
                  hr);
        return false;
      }
    }

    return true;
  }

  HRESULT GetBackbuffer(IDirect3DDevice9* device, IDirect3DSurface9** surface) {
    static bool use_backbuffer = false;
    static bool checked_exceptions = false;

    if (!checked_exceptions) {
      if (umu::apppath_t::GetProgramBaseName() == _T("hotd_ng.exe")) {
        use_backbuffer = true;
      }
      checked_exceptions = true;
    }

    if (use_backbuffer) {
      return device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, surface);
    } else {
      return device->GetRenderTarget(0, surface);
    }
  }

  bool GetSwapDesc(D3DPRESENT_PARAMETERS* pp) {
    CComPtr<IDirect3DSwapChain9> swap;
    HRESULT hr = device_->GetSwapChain(0, &swap);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": !GetSwapChain(), #0x%08X\n", hr);
      return false;
    }

    hr = swap->GetPresentParameters(pp);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": !GetPresentParameters(), #0x%08X\n", hr);
      return false;
    }

    return true;
  }

  bool InitFormatBackbuffer() {
    D3DPRESENT_PARAMETERS pp;

    if (!GetSwapDesc(&pp)) {
      return false;
    }

    CComPtr<IDirect3DSurface9> back_buffer;
    HRESULT hr = device_->GetRenderTarget(0, &back_buffer);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": !GetRenderTarget(), #0x%08X\n", hr);
    } else {
      D3DSURFACE_DESC desc;
      hr = back_buffer->GetDesc(&desc);
      if (FAILED(hr)) {
        ATLTRACE2(atlTraceException, 0, __FUNCTION__ ": !GetDesc(), #0x%08X\n",
                  hr);
      } else {
        d3d9_format_ = desc.Format;
        cx_ = desc.Width;
        cy_ = desc.Height;

        return true;
      }
    }

    d3d9_format_ = pp.BackBufferFormat;
    cx_ = pp.BackBufferWidth;
    cy_ = pp.BackBufferHeight;
    return true;
  }

  bool CreateSharedMemory(size_t data_size) noexcept {
    bool need_create_file_mapping = false;
    if (nullptr == shared_frames_) {
      need_create_file_mapping = true;
    } else if (data_size * 2 > shared_frames_.GetMappingSize()) {
      shared_frames_.Unmap();
      need_create_file_mapping = true;
    }

    if (need_create_file_mapping) {
      HRESULT hr = shared_frames_.MapSharedMem(
          sizeof(SharedVideoYuvFrames) + data_size * kNumberOfSharedFrames,
          kSharedVideoYuvFramesFileMappingName.data(), nullptr,
          HookD3D9::GetInstance().SA());
      if (FAILED(hr)) {
        ATLTRACE2(atlTraceException, 0, "MapSharedMem() failed.\n");
        return false;
      }

      HANDLE ev = CreateEvent(HookD3D9::GetInstance().SA(), FALSE, FALSE,
                              kSharedVideoFrameReadyEventName.data());
      if (nullptr == ev) {
        ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
                  GetLastError());
        return false;
      }
      shared_frame_ready_event_.Attach(ev);

      ATLTRACE2(
          atlTraceUtil, 0, "MapSharedMem size = %zu + %zu * 2 = %zu\n",
          sizeof(SharedVideoYuvFrames), data_size,
          sizeof(SharedVideoYuvFrames) + data_size * kNumberOfSharedFrames);

      auto shared_frame =
          static_cast<SharedVideoYuvFrames*>(shared_frames_.GetData());
      shared_frame->data_size = static_cast<uint32_t>(data_size);
    }
    return true;
  }

  PackedVideoYuvFrame* GetPackedVideoYuvFrame(size_t index) noexcept {
    auto shared_frame =
        static_cast<SharedVideoYuvFrames*>(shared_frames_.GetData());
    return reinterpret_cast<PackedVideoYuvFrame*>(
        static_cast<char*>(shared_frames_) + sizeof(SharedVideoYuvFrames) +
        index * shared_frame->data_size);
  }

  void FreeSharedMemory() noexcept {
    shared_frames_.Unmap();
  }

  int WaitingThread() {
    for (;;) {
      HANDLE started_events[] = {stop_event_, encoder_started_event_};
      DWORD wait = WaitForMultipleObjects(_countof(started_events),
                                          started_events, FALSE, INFINITE);
      if (WAIT_OBJECT_0 == wait) {
        ATLTRACE2(atlTraceUtil, 0, "%s: stopping.\n", __func__);
        break;
      } else if (WAIT_OBJECT_0 + 1 == wait) {
        is_encoder_started_ = true;
        ATLTRACE2(atlTraceUtil, 0, "%s: Video encoder is started.\n", __func__);
      } else {
        int error_code = GetLastError();
        ATLTRACE2(atlTraceException, 0,
                  "%s: WaitForMultipleObjects() return %u, error %u.\n",
                  __func__, wait, error_code);
        return error_code;
      }

      wait = WaitForSingleObject(donot_present_event_, 0);
      if (WAIT_OBJECT_0 == wait) {
        donot_present_ = true;
      } else {
        donot_present_ = false;
      }

      HANDLE stop_events[] = {stop_event_, encoder_stopped_event_};
      wait = WaitForMultipleObjects(_countof(stop_events), stop_events, FALSE,
                                    INFINITE);
      donot_present_ = false;
      if (WAIT_OBJECT_0 == wait) {
        ATLTRACE2(atlTraceUtil, 0, "%s: stopping.\n", __func__);
        break;
      } else if (WAIT_OBJECT_0 + 1 == wait) {
        is_encoder_started_ = false;
        ATLTRACE2(atlTraceUtil, 0, "%s: Video encoder is stopped.\n", __func__);
      } else {
        int error_code = GetLastError();
        ATLTRACE2(atlTraceException, 0,
                  "%s: WaitForMultipleObjects() return %u, error %u.\n",
                  __func__, wait, error_code);
        return error_code;
      }
    }
    return 0;
  }

 private:
  bool initialized_{false};
  bool should_update_{false};

  std::atomic<bool> is_encoder_started_{false};
  std::atomic<bool> donot_present_{false};
  std::thread waiting_thread_;
  CHandle encoder_started_event_;
  CHandle encoder_stopped_event_;
  CHandle donot_present_event_;
  CHandle stop_event_;

  IDirect3DDevice9* device_ = nullptr;
  D3DFORMAT d3d9_format_;
  uint32_t cx_ = 0;
  uint32_t cy_ = 0;
  CComPtr<IDirect3DSurface9> copy_surface_;
  CComPtr<IDirect3DQuery9> query_;

  size_t frame_count_{0};

  CAtlFileMapping<SharedVideoFrameInfo> shared_frame_info_;
  CAtlFileMapping<char> shared_frames_;
  CHandle shared_frame_ready_event_;
};
