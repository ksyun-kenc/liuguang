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

#include <d3d9.h>

class CaptureD3d9 {
 public:
  void Capture(IDirect3DDevice9* device,
               IDirect3DSurface9* backbuffer) noexcept;

  void Reset() noexcept {
    ATLTRACE2(atlTraceUtil, 0, "%s: +\n", __func__);
    setup_ = false;
    reset_ = true;
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
    ATLTRACE2(atlTraceUtil, 0, "%s: %u * %u, %u\n", __func__, width_, height_,
              d3d9_format_);

    HRESULT hr = device_->CreateOffscreenPlainSurface(
        width_, height_, d3d9_format_, D3DPOOL_SYSTEMMEM, &copy_surface_,
        nullptr);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                "%s: !CreateOffscreenPlainSurface(), #0x%08X\n", __func__, hr);
      return false;
    }

    ATLTRACE2(atlTraceUtil, 0, "%s: -\n", __func__);
    return true;
  }

  HRESULT GetBackbuffer(IDirect3DDevice9* device, IDirect3DSurface9** surface);

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

  bool InitFormatBackbuffer() noexcept;

 private:
  bool setup_{false};
  bool reset_{false};

  IDirect3DDevice9* device_ = nullptr;
  D3DFORMAT d3d9_format_;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  CComPtr<IDirect3DSurface9> copy_surface_;
  HWND window_ = nullptr;
};
