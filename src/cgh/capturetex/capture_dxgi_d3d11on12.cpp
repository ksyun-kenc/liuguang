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

#include "capture_dxgi_d3d11on12.h"

#include <d3d11on12.h>

#include "capturetex.h"
#include "d3d_utils.h"

#include "umu/time_measure.hpp"

namespace capture_d3d11on12 {

using namespace regame;

namespace {
constexpr size_t kMaxBackbuffers = 8;

bool initialized_ = false;
bool is_dxgi_1_4_ = false;

ID3D12Device* device_ = nullptr;
PFN_D3D11ON12_CREATE_DEVICE D3D11On12CreateDevice_ = nullptr;

ID3D11Device* device11_ = nullptr;
ID3D11DeviceContext* context11_ = nullptr;
ID3D11On12Device* device11on12_ = nullptr;

ID3D11Resource* backbuffer11_[kMaxBackbuffers]{};
size_t backbuffer_count_;
size_t current_backbuffer_;

DXGI_FORMAT format_;
UINT width_ = 0;
UINT height_ = 0;
bool multisampled_ = false;
HWND window_ = nullptr;

HRESULT InitFormat(IDXGISwapChain* swap) {
  CComPtr<IDXGISwapChain3> swap3;
  HRESULT hr = swap->QueryInterface(IID_PPV_ARGS(&swap3));
  if (SUCCEEDED(hr)) {
    is_dxgi_1_4_ = true;
    swap3.Release();
  } else {
    is_dxgi_1_4_ = false;
  }
  ATLTRACE2(atlTraceUtil, 0, "Is DXGI 1.4? %d\n", is_dxgi_1_4_);

  DXGI_SWAP_CHAIN_DESC desc;
  hr = swap->GetDesc(&desc);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "!GetDesc(), #0x%08X\n", hr);
    return hr;
  }

  format_ = desc.BufferDesc.Format;
  width_ = desc.BufferDesc.Width;
  height_ = desc.BufferDesc.Height;
  multisampled_ = desc.SampleDesc.Count > 1;
  window_ = desc.OutputWindow;
  ATLTRACE2(atlTraceUtil, 0,
            "Format %u, %u * %u, Count %u, OutputWindow %p, Windowed %d\n",
            format_, width_, height_, desc.SampleDesc.Count, window_,
            desc.Windowed);

  backbuffer_count_ =
      desc.SwapEffect == DXGI_SWAP_EFFECT_DISCARD ? 1 : desc.BufferCount;
  if (1 == backbuffer_count_) {
    is_dxgi_1_4_ = false;
  }

  if (kMaxBackbuffers < backbuffer_count_) {
    ATLTRACE2(atlTraceException, 0, "BackbufferCount = %u\n",
              backbuffer_count_);
    backbuffer_count_ = 1;
    is_dxgi_1_4_ = false;
  }

  ID3D12Resource* backbuffer12[kMaxBackbuffers]{};
  D3D11_RESOURCE_FLAGS resource_flags = {};
  for (UINT i = 0; i < backbuffer_count_; ++i) {
    hr = swap->GetBuffer(i, IID_PPV_ARGS(&backbuffer12[i]));
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "!GetBuffer(), #0x%08x\n", hr);
      return hr;
    }
    hr = device11on12_->CreateWrappedResource(
        backbuffer12[i], &resource_flags, D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_PRESENT, IID_PPV_ARGS(&backbuffer11_[i]));
    backbuffer12[i]->Release();
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "!CreateWrappedResource(), #0x%08x\n",
                hr);
      return hr;
    }
    device11on12_->ReleaseWrappedResources(&backbuffer11_[i], 1);
  }

  return hr;
}

HRESULT Initialize(IDXGISwapChain* swap) {
  HRESULT hr = swap->GetDevice(__uuidof(ID3D12Device),
                               reinterpret_cast<void**>(&device_));
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "%s: Failed to get device from swap, #0x%08X\n", __func__, hr);
    return hr;
  }
  device_->Release();

  if (nullptr == D3D11On12CreateDevice_) {
    D3D11On12CreateDevice_ =
        reinterpret_cast<PFN_D3D11ON12_CREATE_DEVICE>(GetProcAddress(
            GetModuleHandle(_T("d3d11.dll")), "D3D11On12CreateDevice"));
    if (nullptr == D3D11On12CreateDevice_) {
      ATLTRACE2(atlTraceException, 0,
                "!GetProcAddress(D3D11On12CreateDevice), #%d\n",
                GetLastError());
      return E_NOINTERFACE;
    }
    ATLTRACE2(atlTraceUtil, 0, "D3D11On12CreateDevice = 0x%p\n",
              D3D11On12CreateDevice_);
  }

  hr = D3D11On12CreateDevice_(device_, 0, nullptr, 0, nullptr, 0, 0, &device11_,
                              &context11_, nullptr);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "!D3D11On12CreateDevice(), #0x%08X\n", hr);
    return hr;
  }

  hr = device11_->QueryInterface(IID_PPV_ARGS(&device11on12_));
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "!QueryInterface(ID3D11On12Device), #0x%08X\n", hr);
    return hr;
  }

  hr = InitFormat(swap);
  return hr;
}

void FreeResource() {
  for (size_t i = 0; i < backbuffer_count_; ++i) {
    if (nullptr != backbuffer11_[i]) {
      backbuffer11_[i]->Release();
    }
  }
  backbuffer_count_ = 0;

  if (nullptr != device11_) {
    device11_->Release();
    device11_ = nullptr;
  }
  if (nullptr != context11_) {
    context11_->Release();
    context11_ = nullptr;
  }
  if (nullptr != device11on12_) {
    device11on12_->Release();
    device11on12_ = nullptr;
  }
}
}  // namespace

void Capture(void* swap, void*) {
  bool should_update = false;
  if (!initialized_) {
    HRESULT hr = Initialize(static_cast<IDXGISwapChain*>(swap));
    if (FAILED(hr)) {
      return;
    }
    initialized_ = true;
    should_update = true;
  }

  LARGE_INTEGER tick;
  QueryPerformanceCounter(&tick);

  size_t index = current_backbuffer_;
  if (is_dxgi_1_4_) {
    IDXGISwapChain3* swap3 = reinterpret_cast<IDXGISwapChain3*>(swap);
    index = swap3->GetCurrentBackBufferIndex();
    if (++current_backbuffer_ >= backbuffer_count_) {
      index = 0;
    }
  }

  ID3D11Resource* backbuffer = backbuffer11_[index];
  device11on12_->AcquireWrappedResources(&backbuffer, 1);

  D3D11_TEXTURE2D_DESC desc;
  CComPtr<ID3D11Texture2D> new_texture;
  HRESULT hr =
      CaptureTexture(device11_, context11_, backbuffer, desc, new_texture);
  device11on12_->ReleaseWrappedResources(&backbuffer, 1);
  context11_->Flush();
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

  if (!g_capture_tex.ShareTexture(new_texture, device11_, context11_)) {
    return;
  }
  stats.elapsed.total = umu::TimeMeasure::Delta(stats.timestamp);

  auto frame = g_capture_tex.GetAvailablePackedVideoTextureFrame();
  frame->stats.timestamp = stats.timestamp;
  frame->stats.elapsed.preprocess = stats.elapsed.preprocess;
  frame->stats.elapsed.total = stats.elapsed.total;
  g_capture_tex.SetSharedFrameReadyEvent();
}

void Free() {
  FreeResource();
  initialized_ = false;
}

}  // namespace capture_d3d11on12
