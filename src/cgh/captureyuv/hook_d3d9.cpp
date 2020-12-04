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

#include "hook_d3d9.h"

#include "captureyuv.h"

#include "umu/apppath_t.h"
#include "umu/memory.h"
#include "umu/time_measure.hpp"

#include "yuv/yuv.h"

namespace {
class CaptureD3d9 {
 public:
  void Capture(IDirect3DDevice9* device,
               IDirect3DSurface9* backbuffer) noexcept {
    if (!CaptureYuv::GetInstance().IsEncoderStarted()) {
      return;
    }

    bool should_update = false;

    if (!setup_) {
      setup_ = Setup(device);
      if (!setup_) {
        ATLTRACE2(atlTraceException, 0, "%s: !Setup()\n", __func__);
        return;
      }
      should_update = true;
    } else if (device != device_) {
      ATLTRACE2(atlTraceException, 0, "%s: old = %p, new = %p\n", __func__,
                device_, device);
      Reset();
      return;
    }

    LARGE_INTEGER tick;
    QueryPerformanceCounter(&tick);

    HRESULT hr = device_->GetRenderTargetData(backbuffer, copy_surface_);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": !GetRenderTargetData(), #0x%08X\n", hr);
      Reset();
      return;
    }

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

    if (should_update) {
      SharedVideoFrameInfo* svfi =
          CaptureYuv::GetInstance().GetSharedVideoFrameInfo();
      svfi->timestamp = tick.QuadPart;
      svfi->type = VideoFrameType::YUV;
      svfi->width = desc.Width;
      svfi->height = desc.Height;
      svfi->format = desc.Format;
    }

    VideoFrameStats stats = {};
    stats.timestamp = tick.QuadPart;
    stats.elapsed.preprocess = umu::TimeMeasure::Delta(tick.QuadPart);
    ATLTRACE2(atlTraceUtil, 0, __FUNCTION__ ": stats.elapsed.prepare = %llu.\n",
              stats.elapsed.preprocess);

    // wait_rgb_mapping = 0
    D3DLOCKED_RECT mapped_rect = {};
    {
      umu::TimeMeasure tm(stats.elapsed.rgb_mapping);
      // LockRect may spend long time
      // ATLTRACE2(atlTraceUtil, 0, __FUNCTION__ ": LockRect +\n");
      hr = copy_surface_->LockRect(&mapped_rect, nullptr, D3DLOCK_READONLY);
      // ATLTRACE2(atlTraceUtil, 0, __FUNCTION__ ": LockRect -, 0x%08X\n", hr);
    }
    ATLTRACE2(atlTraceUtil, 0, "frame[%zd] stats.elapsed.rgb_mapping = %llu.\n",
              CaptureYuv::GetInstance().GetFrameCount(),
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
    if (!CaptureYuv::GetInstance().CreateSharedVideoYuvFrames(data_size)) {
      return;
    }

    auto frame = CaptureYuv::GetInstance().GetAvailablePackedVideoYuvFrame();
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
        atlTraceUtil, 0, "frame[%zd] stats.elapsed.yuv_convert = %llu, \n",
        CaptureYuv::GetInstance().GetFrameCount(), stats.elapsed.yuv_convert);

    stats.elapsed.total = umu::TimeMeasure::Delta(stats.timestamp);

    frame->stats.timestamp = stats.timestamp;
    frame->stats.elapsed.preprocess = stats.elapsed.preprocess;
    frame->stats.elapsed.nvenc = 0;
    frame->stats.elapsed.wait_rgb_mapping = stats.elapsed.wait_rgb_mapping;
    frame->stats.elapsed.rgb_mapping = stats.elapsed.rgb_mapping;
    frame->stats.elapsed.yuv_convert = stats.elapsed.yuv_convert;
    frame->stats.elapsed.total = stats.elapsed.total;

    CaptureYuv::GetInstance().SetSharedFrameReadyEvent();
  }

  void Reset() noexcept {
    ATLTRACE2(atlTraceUtil, 0, "%s: +\n", __func__);
    setup_ = false;
    copy_surface_.Release();
    device_ = nullptr;
    ATLTRACE2(atlTraceUtil, 0, "%s: -\n", __func__);
  }

  void Release(IDirect3DDevice9* device) noexcept {
    if (device == device_) {
      Reset();
    }
  }

  void PresentBegin(IDirect3DDevice9* device,
                    IDirect3DSurface9** backbuffer) noexcept {
    HRESULT hr = GetBackbuffer(device, backbuffer);
    if (SUCCEEDED(hr)) {
      Capture(device, *backbuffer);
    } else {
      // D3DERR_INVALIDCALL = 0x8876086C
      ATLTRACE2(atlTraceException, 0, "%s: !GetBackbuffer(), #0x%08X\n",
                __func__, hr);
    }
  }

  void PresentEnd(IDirect3DDevice9* device,
                  IDirect3DSurface9* backbuffer) noexcept {
    if (nullptr != backbuffer) {
      backbuffer->Release();
    }
  }

 private:
  bool Setup(IDirect3DDevice9* device) {
    ATLTRACE2(atlTraceUtil, 0, "%s: +\n", __func__);
    device_ = device;

    if (!InitFormatBackbuffer()) {
      return false;
    }
    ATLTRACE2(atlTraceUtil, 0, "%s: %u * %u, %u\n", __func__, cx_, cy_,
              d3d9_format_);

    HRESULT hr = device_->CreateOffscreenPlainSurface(
        cx_, cy_, d3d9_format_, D3DPOOL_SYSTEMMEM, &copy_surface_, nullptr);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                "%s: !CreateOffscreenPlainSurface(), #0x%08X\n", __func__, hr);
      return false;
    }

    ATLTRACE2(atlTraceUtil, 0, "%s: -\n", __func__);
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
      ATLTRACE2(atlTraceException, 0, "%s: !GetSwapChain(), #0x%08X\n",
                __func__, hr);
      return false;
    }

    hr = swap->GetPresentParameters(pp);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "%s: !GetPresentParameters(), #0x%08X\n",
                __func__, hr);
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
      ATLTRACE2(atlTraceException, 0, "%s: !GetRenderTarget(), #0x%08X\n",
                __func__, hr);
    } else {
      D3DSURFACE_DESC desc;
      hr = back_buffer->GetDesc(&desc);
      if (FAILED(hr)) {
        ATLTRACE2(atlTraceException, 0, "%s: !GetDesc(), #0x%08X\n", __func__,
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

 private:
  bool setup_{false};

  IDirect3DDevice9* device_ = nullptr;
  D3DFORMAT d3d9_format_;
  uint32_t cx_ = 0;
  uint32_t cy_ = 0;
  CComPtr<IDirect3DSurface9> copy_surface_;
};

CaptureD3d9 capture;
}  // namespace

IDIRECT3DDEVICE9EX_RELEASE HookD3d9::IDirect3DDevice9Ex_Release_;

IDIRECT3DSWAPCHAIN9_PRESENT HookD3d9::IDirect3DSwapChain9_Present_;
IDIRECT3DDEVICE9EX_RESET HookD3d9::IDirect3DDevice9Ex_Reset_;
IDIRECT3DDEVICE9EX_PRESENT HookD3d9::IDirect3DDevice9Ex_Present_;
IDIRECT3DDEVICE9EX_PRESENTEX HookD3d9::IDirect3DDevice9Ex_PresentEx_;
IDIRECT3DDEVICE9EX_RESETEX HookD3d9::IDirect3DDevice9Ex_ResetEx_;

bool HookD3d9::Hook() noexcept {
  ATLTRACE2(atlTraceUtil, 0, "%s: +\n", __func__);
  HMODULE d3d9_module = GetModuleHandle(_T("d3d9.dll"));
  if (nullptr == d3d9_module) {
    ATLTRACE2(atlTraceUtil, 0, "%s: d3d9.dll not loaded.\n", __func__);
    return false;
  }

  auto Direct3DCreate9Ex =
      reinterpret_cast<HRESULT(WINAPI*)(UINT, IDirect3D9Ex**)>(
          GetProcAddress(d3d9_module, "Direct3DCreate9Ex"));
  if (nullptr == Direct3DCreate9Ex) {
    ATLTRACE2(atlTraceException, 0, "%s: !Direct3DCreate9Ex, #%d\n", __func__,
              GetLastError());
    return false;
  }

  CComPtr<IDirect3D9Ex> d3d9ex;
  HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "%s: !Direct3DCreate9Ex(), #0x%08X\n",
              __func__, hr);
    return false;
  }

  HWND hwnd = CreateWindowEx(0, L"Static", L"d3d9 temporary window", WS_POPUP,
                             0, 0, 2, 2, nullptr, nullptr, nullptr, nullptr);
  if (nullptr == hwnd) {
    ATLTRACE2(atlTraceException, 0, "!CreateWindowEx(), #%d\n", GetLastError());
    return false;
  }
  BOOST_SCOPE_EXIT_ALL(&) { DestroyWindow(hwnd); };

  CComPtr<IDirect3DDevice9Ex> device;
  D3DPRESENT_PARAMETERS pp = {};
  pp.Windowed = TRUE;
  pp.SwapEffect = D3DSWAPEFFECT_FLIP;
  pp.BackBufferFormat = D3DFMT_A8R8G8B8;
  pp.BackBufferWidth = 2;
  pp.BackBufferHeight = 2;
  pp.BackBufferCount = 1;
  pp.hDeviceWindow = hwnd;
  pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
  hr = d3d9ex->CreateDeviceEx(
      D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
      D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_NOWINDOWCHANGES, &pp,
      nullptr, &device);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "%s: !IDirect3D9Ex::CreateDeviceEx(), #0x%08X\n", __func__, hr);
    return false;
  }

  CComPtr<IDirect3DSwapChain9> swap;
  hr = device->GetSwapChain(0, &swap);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "%s: !IDirect3DDevice9Ex::GetSwapChain(), #0x%08X\n", __func__,
              hr);
    return false;
  }

  // swap->Present;
  IDirect3DSwapChain9_Present_ = reinterpret_cast<IDIRECT3DSWAPCHAIN9_PRESENT>(
      umu::memory::GetVTableFunctionAddress(swap, 3));
  ATLTRACE2(atlTraceUtil, 0, "%s: &IDirect3DSwapChain9::Present = %p\n",
            __func__, IDirect3DSwapChain9_Present_);

  // 2: device->Release;
  IDirect3DDevice9Ex_Release_ = reinterpret_cast<IDIRECT3DDEVICE9EX_RELEASE>(
      umu::memory::GetVTableFunctionAddress(device, 2));
  ATLTRACE2(atlTraceUtil, 0, "%s: &IDirect3DDevice9Ex::Release = %p\n",
            __func__, IDirect3DDevice9Ex_Release_);

  // 16: device->Reset;
  IDirect3DDevice9Ex_Reset_ = reinterpret_cast<IDIRECT3DDEVICE9EX_RESET>(
      umu::memory::GetVTableFunctionAddress(device, 16));
  ATLTRACE2(atlTraceUtil, 0, "%s: &IDirect3DDevice9Ex::Reset = %p\n", __func__,
            IDirect3DDevice9Ex_Reset_);
  // 17: device->Present;
  IDirect3DDevice9Ex_Present_ = reinterpret_cast<IDIRECT3DDEVICE9EX_PRESENT>(
      umu::memory::GetVTableFunctionAddress(device, 17));
  ATLTRACE2(atlTraceUtil, 0, "%s: &IDirect3DDevice9Ex::Present = %p\n",
            __func__, IDirect3DDevice9Ex_Present_);
  // 121: device->PresentEx;
  IDirect3DDevice9Ex_PresentEx_ =
      reinterpret_cast<IDIRECT3DDEVICE9EX_PRESENTEX>(
          umu::memory::GetVTableFunctionAddress(device, 121));
  ATLTRACE2(atlTraceUtil, 0, "%s: &IDirect3DDevice9Ex::PresentEx = %p\n",
            __func__, IDirect3DDevice9Ex_PresentEx_);
  // 132: device->ResetEx;
  IDirect3DDevice9Ex_ResetEx_ = reinterpret_cast<IDIRECT3DDEVICE9EX_RESETEX>(
      umu::memory::GetVTableFunctionAddress(device, 132));
  ATLTRACE2(atlTraceUtil, 0, "%s: &IDirect3DDevice9Ex::ResetEx = %p\n",
            __func__, IDirect3DDevice9Ex_ResetEx_);

  NTSTATUS status = umu::HookAllThread(hook_IDirect3DDevice9Ex_Release_,
                                       IDirect3DDevice9Ex_Release_, MyRelease);
  if (!NT_SUCCESS(status)) {
    ATLTRACE2(atlTraceException, 0,
              "%s: hook IDirect3DDevice9Ex::Release failed, #0x%08X!\n",
              __func__, status);
    return false;
  }

  status = umu::HookAllThread(hook_IDirect3DSwapChain9_Present_,
                              IDirect3DSwapChain9_Present_, MySwapPresent);
  if (!NT_SUCCESS(status)) {
    ATLTRACE2(atlTraceException, 0,
              "%s: hook IDirect3DSwapChain9::Present failed, #0x%08X!\n",
              __func__, status);
    return false;
  }

  status = umu::HookAllThread(hook_IDirect3DDevice9Ex_Reset_,
                              IDirect3DDevice9Ex_Reset_, MyReset);
  if (!NT_SUCCESS(status)) {
    ATLTRACE2(atlTraceException, 0,
              "%s: hook IDirect3DDevice9Ex::Reset failed, #0x%08X!\n", __func__,
              status);
    return false;
  }

  status = umu::HookAllThread(hook_IDirect3DDevice9Ex_Present_,
                              IDirect3DDevice9Ex_Present_, MyPresent);
  if (!NT_SUCCESS(status)) {
    ATLTRACE2(atlTraceException, 0,
              "%s: hook IDirect3DDevice9Ex::Present failed, #0x%08X!\n",
              __func__, status);
    return false;
  }

  status = umu::HookAllThread(hook_IDirect3DDevice9Ex_PresentEx_,
                              IDirect3DDevice9Ex_PresentEx_, MyPresentEx);
  if (!NT_SUCCESS(status)) {
    ATLTRACE2(atlTraceException, 0,
              "%s: hook IDirect3DDevice9Ex::PresentEx failed, #0x%08X!\n",
              __func__, status);
    return false;
  }

  status = umu::HookAllThread(hook_IDirect3DDevice9Ex_ResetEx_,
                              IDirect3DDevice9Ex_ResetEx_, MyResetEx);
  if (!NT_SUCCESS(status)) {
    ATLTRACE2(atlTraceException, 0,
              "%s: hook IDirect3DDevice9Ex::ResetEx failed, #0x%08X!\n",
              __func__, status);
    return false;
  }

  ATLTRACE2(atlTraceUtil, 0, "%s: -\n", __func__);
  return true;
}

void HookD3d9::Unhook() noexcept {
  // not necessary
}

ULONG STDMETHODCALLTYPE HookD3d9::MyRelease(IDirect3DDevice9Ex* This) {
  ULONG rc = IDirect3DDevice9Ex_Release_(This);
  if (rc <= 1) {
    ATLTRACE2(atlTraceUtil, 0, "%s: %u\n", __func__, rc);
    capture.Release(This);
  }
  return rc;
}

HRESULT STDMETHODCALLTYPE HookD3d9::MySwapPresent(IDirect3DSwapChain9* This,
                                                  CONST RECT* pSourceRect,
                                                  CONST RECT* pDestRect,
                                                  HWND hDestWindowOverride,
                                                  CONST RGNDATA* pDirtyRegion,
                                                  DWORD dwFlags) {
  IDirect3DSurface9* backbuffer = nullptr;
  IDirect3DDevice9* device = nullptr;

  HRESULT hr = This->GetDevice(&device);
  if (SUCCEEDED(hr)) {
    device->Release();
  };
  if (nullptr != device) {
    capture.PresentBegin(device, &backbuffer);
  }

  if (CaptureYuv::GetInstance().IsPresentEnabled()) {
    hr = IDirect3DSwapChain9_Present_(This, pSourceRect, pDestRect,
                                      hDestWindowOverride, pDirtyRegion,
                                      dwFlags);
  }

  if (nullptr != device) {
    capture.PresentEnd(device, backbuffer);
  }

  return hr;
}

HRESULT STDMETHODCALLTYPE
HookD3d9::MyReset(IDirect3DDevice9Ex* This,
                  D3DPRESENT_PARAMETERS* pPresentationParameters) {
  capture.Reset();
  return IDirect3DDevice9Ex_Reset_(This, pPresentationParameters);
}

// UMU: SDL calls IDirect3DDevice9Ex::Present
HRESULT STDMETHODCALLTYPE HookD3d9::MyPresent(IDirect3DDevice9Ex* This,
                                              CONST RECT* pSourceRect,
                                              CONST RECT* pDestRect,
                                              HWND hDestWindowOverride,
                                              CONST RGNDATA* pDirtyRegion) {
  HRESULT hr = S_OK;
  IDirect3DSurface9* backbuffer = nullptr;

  capture.PresentBegin(This, &backbuffer);
  if (CaptureYuv::GetInstance().IsPresentEnabled()) {
    hr = IDirect3DDevice9Ex_Present_(This, pSourceRect, pDestRect,
                                     hDestWindowOverride, pDirtyRegion);
  }
  capture.PresentEnd(This, backbuffer);
  return hr;
}

HRESULT STDMETHODCALLTYPE HookD3d9::MyPresentEx(IDirect3DDevice9Ex* This,
                                                CONST RECT* pSourceRect,
                                                CONST RECT* pDestRect,
                                                HWND hDestWindowOverride,
                                                CONST RGNDATA* pDirtyRegion,
                                                DWORD dwFlags) {
  HRESULT hr = S_OK;
  IDirect3DSurface9* backbuffer = nullptr;

  capture.PresentBegin(This, &backbuffer);
  if (CaptureYuv::GetInstance().IsPresentEnabled()) {
    hr = IDirect3DDevice9Ex_PresentEx_(This, pSourceRect, pDestRect,
                                       hDestWindowOverride, pDirtyRegion,
                                       dwFlags);
  }
  capture.PresentEnd(This, backbuffer);
  return hr;
}

HRESULT STDMETHODCALLTYPE
HookD3d9::MyResetEx(IDirect3DDevice9Ex* This,
                    D3DPRESENT_PARAMETERS* pPresentationParameters,
                    D3DDISPLAYMODEEX* pFullscreenDisplayMode) {
  capture.Reset();
  return IDirect3DDevice9Ex_ResetEx_(This, pPresentationParameters,
                                     pFullscreenDisplayMode);
}
