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

#include "capture_dxgi_d3d11.h"

#include <d3d11.h>

#include "capturetex.h"
#include "d3d_utils.h"

#include "umu/time_measure.hpp"

namespace capture_d3d11 {

using namespace regame;

namespace {
bool initialized_ = false;
ID3D11Device* device_ = nullptr;
ID3D11DeviceContext* context_ = nullptr;

// feature_level >= D3D_FEATURE_LEVEL_11_1
PFN_D3D11_CREATE_DEVICE D3D11CreateDevice_;
ID3D11Device* device11_ = nullptr;
ID3D11DeviceContext* context11_ = nullptr;

UINT width_ = 0;
UINT height_ = 0;
HWND window_ = nullptr;

HRESULT CreateD3D11DeviceOnLevel11_1() {
  if (nullptr == D3D11CreateDevice_) {
    D3D11CreateDevice_ = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(
        GetProcAddress(GetModuleHandle(_T("d3d11.dll")), "D3D11CreateDevice"));
    if (nullptr == D3D11CreateDevice_) {
      ATLTRACE2(atlTraceException, 0,
                "!GetProcAddress(D3D11CreateDevice), #%d\n", GetLastError());
      return E_NOINTERFACE;
    }
    ATLTRACE2(atlTraceUtil, 0, "D3D11CreateDevice = 0x%p\n",
              D3D11CreateDevice_);
  }

  D3D_DRIVER_TYPE driver_types[]{
      D3D_DRIVER_TYPE_HARDWARE,
      // D3D_DRIVER_TYPE_WARP,
      // D3D_DRIVER_TYPE_REFERENCE,
  };

  D3D_FEATURE_LEVEL feature_level;
  D3D_FEATURE_LEVEL feature_levels[]{
      D3D_FEATURE_LEVEL_11_1,
      D3D_FEATURE_LEVEL_12_0,
      D3D_FEATURE_LEVEL_12_1,
  };

  HRESULT hr;
  for (int driver_type_index = 0; driver_type_index < ARRAYSIZE(driver_types);
       driver_type_index++) {
    hr = D3D11CreateDevice_(NULL, driver_types[driver_type_index], NULL, 0,
                            feature_levels, ARRAYSIZE(feature_levels),
                            D3D11_SDK_VERSION, &device11_, &feature_level,
                            &context11_);
    if (SUCCEEDED(hr)) {
      ATLTRACE2(atlTraceUtil, 0, "D3D11CreateDevice() driver type %d\n",
                driver_types[driver_type_index]);
      break;
    }
  }

  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "!D3D11CreateDevice(), #0x%08X\n", hr);
    return hr;
  }

  return S_OK;
}

HRESULT Initialize(IDXGISwapChain* swap) {
  HRESULT hr = swap->GetDevice(__uuidof(ID3D11Device),
                               reinterpret_cast<void**>(&device_));
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "%s: Failed to get device from swap, #0x%08X\n", __func__, hr);
    return hr;
  }

  device_->GetImmediateContext(&context_);
  ULONG ref = context_->Release();
  ATLTRACE2(atlTraceUtil, 0, "ID3D11DeviceContext::Release() = %d\n", ref);
  ref = device_->Release();
  ATLTRACE2(atlTraceUtil, 0, "ID3D11Device::Release() = %d\n", ref);

  DXGI_SWAP_CHAIN_DESC desc;
  hr = swap->GetDesc(&desc);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "%s: Failed to get desc from swap, #0x%08X\n", __func__, hr);
    return hr;
  }
  window_ = desc.OutputWindow;

  hr = CreateD3D11DeviceOnLevel11_1();
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "%s: Failed to CreateD3D11DeviceOnLevel11_1, #0x%08X\n", __func__,
              hr);
    return hr;
  }
  return hr;
}
}  // namespace

void Capture(void* swap, void* backbuffer) {
  bool should_update = false;
  if (!initialized_) {
    HRESULT hr = Initialize(static_cast<IDXGISwapChain*>(swap));
    if (FAILED(hr)) {
      return;
    }
    initialized_ = true;
    should_update = true;
  }

  auto dxgi_backbuffer = static_cast<IDXGIResource*>(backbuffer);
  CComQIPtr<ID3D11Texture2D> acquired_texture(dxgi_backbuffer);
  if (!acquired_texture) {
    ATLTRACE2(atlTraceException, 0, "!QueryInterface(ID3D11Texture2D)\n");
    return;
  }

  LARGE_INTEGER tick;
  QueryPerformanceCounter(&tick);

  D3D11_TEXTURE2D_DESC desc;
  CComPtr<ID3D11Texture2D> new_texture;
  HRESULT hr =
      CaptureTexture(device_, context_, acquired_texture, desc, new_texture);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "!CaptureTexture(), #0x%08X\n", hr);
    return;
  }

  if (should_update) {
    SharedVideoFrameInfo* svfi = g_capture_tex.GetSharedVideoFrameInfo();
    svfi->timestamp = tick.QuadPart;
    svfi->type = VideoFrameType::kTexture;
    svfi->width = desc.Width;
    svfi->height = desc.Height;
    svfi->format = desc.Format;
    svfi->window = reinterpret_cast<std::uint64_t>(window_);
  }

  VideoFrameStats stats = {};
  stats.timestamp = tick.QuadPart;
  stats.elapsed.preprocess = umu::TimeMeasure::Delta(tick.QuadPart);

  if (!g_capture_tex.CreateSharedVideoTextureFrames()) {
    return;
  }

  if (!g_capture_tex.ShareTexture(new_texture, device_, context_)) {
    return;
  }
  stats.elapsed.total = umu::TimeMeasure::Delta(stats.timestamp);

  auto frame = g_capture_tex.GetAvailablePackedVideoTextureFrame();
  frame->stats.timestamp = stats.timestamp;
  frame->stats.elapsed.preprocess = stats.elapsed.preprocess;
  frame->stats.elapsed.total = stats.elapsed.total;
  g_capture_tex.SetSharedFrameReadyEvent();
}

void FreeResource() {
  if (nullptr != device11_) {
    device11_->Release();
    device11_ = nullptr;
  }
  if (nullptr != context11_) {
    context11_->Release();
    context11_ = nullptr;
  }
}

void Free() {
  g_capture_tex.FreeSharedTexture();
  FreeResource();
  initialized_ = false;
}

}  // namespace capture_d3d11
