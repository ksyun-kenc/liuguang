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

#include "hook_dxgi.h"

#include <d3d11.h>
#include <d3d11on12.h>
#include <d3d12.h>

#include "captureyuv.h"

#include "umu/memory.h"
#include "umu/time_measure.hpp"

#include "yuv/yuv.h"

namespace {
DXGI_FORMAT EnsureNotTypeless(DXGI_FORMAT format) noexcept {
  // Assumes UNORM or FLOAT; doesn't use UINT or SINT
  switch (format) {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
      return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case DXGI_FORMAT_R32G32B32_TYPELESS:
      return DXGI_FORMAT_R32G32B32_FLOAT;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
      return DXGI_FORMAT_R16G16B16A16_UNORM;
    case DXGI_FORMAT_R32G32_TYPELESS:
      return DXGI_FORMAT_R32G32_FLOAT;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
      return DXGI_FORMAT_R10G10B10A2_UNORM;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_R16G16_TYPELESS:
      return DXGI_FORMAT_R16G16_UNORM;
    case DXGI_FORMAT_R32_TYPELESS:
      return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_R8G8_TYPELESS:
      return DXGI_FORMAT_R8G8_UNORM;
    case DXGI_FORMAT_R16_TYPELESS:
      return DXGI_FORMAT_R16_UNORM;
    case DXGI_FORMAT_R8_TYPELESS:
      return DXGI_FORMAT_R8_UNORM;
    case DXGI_FORMAT_BC1_TYPELESS:
      return DXGI_FORMAT_BC1_UNORM;
    case DXGI_FORMAT_BC2_TYPELESS:
      return DXGI_FORMAT_BC2_UNORM;
    case DXGI_FORMAT_BC3_TYPELESS:
      return DXGI_FORMAT_BC3_UNORM;
    case DXGI_FORMAT_BC4_TYPELESS:
      return DXGI_FORMAT_BC4_UNORM;
    case DXGI_FORMAT_BC5_TYPELESS:
      return DXGI_FORMAT_BC5_UNORM;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
      return DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
      return DXGI_FORMAT_B8G8R8X8_UNORM;
    case DXGI_FORMAT_BC7_TYPELESS:
      return DXGI_FORMAT_BC7_UNORM;
    default:
      return format;
  }
}

HRESULT CaptureTexture(_In_ ID3D11Device* device,
                       _In_ ID3D11DeviceContext* context,
                       _In_ ID3D11Resource* source,
                       D3D11_TEXTURE2D_DESC& desc,
                       CComPtr<ID3D11Texture2D>& staging) noexcept {
  assert(nullptr != context);
  assert(nullptr != source);

  if (nullptr == context || nullptr == source) {
    return E_INVALIDARG;
  }

  D3D11_RESOURCE_DIMENSION type = D3D11_RESOURCE_DIMENSION_UNKNOWN;
  source->GetType(&type);

  if (D3D11_RESOURCE_DIMENSION_TEXTURE2D != type) {
    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
  }

  CComPtr<ID3D11Texture2D> acquired_texture;
  HRESULT hr = source->QueryInterface(IID_PPV_ARGS(&acquired_texture));
  if (FAILED(hr)) {
    return hr;
  }

  assert(acquired_texture);
  acquired_texture->GetDesc(&desc);

  if (desc.SampleDesc.Count > 1) {
    // MSAA content must be resolved before being copied to a staging texture
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    CComPtr<ID3D11Texture2D> new_texture;
    hr = device->CreateTexture2D(&desc, nullptr, &new_texture);
    if (FAILED(hr)) {
      return hr;
    }
    assert(new_texture);

    DXGI_FORMAT format = EnsureNotTypeless(desc.Format);
    // ATLTRACE2(atlTraceUtil, 0, "%u -> %u\n", desc.Format, format);

    UINT support = 0;
    hr = device->CheckFormatSupport(format, &support);
    if (FAILED(hr)) {
      return hr;
    }
    if (!(support & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE)) {
      return E_FAIL;
    }

    for (UINT item = 0; item < desc.ArraySize; ++item) {
      for (UINT level = 0; level < desc.MipLevels; ++level) {
        UINT index = D3D11CalcSubresource(level, item, desc.MipLevels);
        context->ResolveSubresource(new_texture, index, source, index, format);
      }
    }

    desc.BindFlags = 0;
    desc.MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    hr = device->CreateTexture2D(&desc, nullptr, &staging);
    if (FAILED(hr))
      return hr;

    assert(staging);

    context->CopyResource(staging, new_texture);
  } else if ((desc.Usage == D3D11_USAGE_STAGING) &&
             (desc.CPUAccessFlags & D3D11_CPU_ACCESS_READ)) {
    // Handle case where the source is already a staging texture we can use
    // directly
    staging = acquired_texture;
  } else {
    // Otherwise, create a staging texture from the non-MSAA source
    desc.BindFlags = 0;
    desc.MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.Usage = D3D11_USAGE_STAGING;
    // ATLTRACE2(atlTraceUtil, 0, "Format = %u\n", desc.Format);

    hr = device->CreateTexture2D(&desc, nullptr, &staging);
    if (FAILED(hr))
      return hr;

    assert(staging);

    context->CopyResource(staging, source);
  }

  return S_OK;
}

class CaptureD3d11 {
 public:
  static CaptureD3d11& GetInstance() {
    static CaptureD3d11 instance;
    return instance;
  }

  static void Capture(void* swap, void* backbuffer) {
    bool should_update = false;
    if (!GetInstance().initialized_) {
      HRESULT hr = GetInstance().Initialize(static_cast<IDXGISwapChain*>(swap));
      if (FAILED(hr)) {
        return;
      }
      GetInstance().initialized_ = true;
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
    HRESULT hr = CaptureTexture(GetInstance().device_, GetInstance().context_,
                                acquired_texture, desc, new_texture);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "!CaptureTexture(), #0x%08X\n", hr);
      return;
    }

    if (should_update) {
      SharedVideoFrameInfo* svfi =
          CaptureYuv::GetInstance().GetSharedVideoFrameInfo();
      svfi->timestamp = tick.QuadPart;
      svfi->type = VideoFrameType::kYuv;
      svfi->width = desc.Width;
      svfi->height = desc.Height;
      svfi->format = desc.Format;
      svfi->window = reinterpret_cast<std::uint64_t>(GetInstance().window_);
    }

    CComPtr<IDXGISurface> surface;
    hr = new_texture->QueryInterface(IID_PPV_ARGS(&surface));
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "!QueryInterface(IDXGISurface)\n");
      return;
    }

    VideoFrameStats stats = {};
    stats.timestamp = tick.QuadPart;
    stats.elapsed.preprocess = umu::TimeMeasure::Delta(tick.QuadPart);

    DXGI_SURFACE_DESC sd = {};
    hr = surface->GetDesc(&sd);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "%p->GetDesc() failed with 0x%08X.\n",
                surface, hr);
      return;
    }

    DXGI_MAPPED_RECT mapped_rect = {};
    {
      umu::TimeMeasure tm(stats.elapsed.rgb_mapping);
      hr = surface->Map(&mapped_rect, DXGI_MAP_READ);
    }
    ATLTRACE2(atlTraceUtil, 0, "frame[%zd] stats.elapsed.rgb_mapping = %llu.\n",
              CaptureYuv::GetInstance().GetFrameCount(),
              stats.elapsed.rgb_mapping);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "%p->Map() failed with 0x%08X.\n",
                surface, hr);
      if (DXGI_ERROR_DEVICE_REMOVED == hr || DXGI_ERROR_DEVICE_RESET == hr) {
        Free();
      }
      return;
    }
    BOOST_SCOPE_EXIT_ALL(&surface) {
      // ATLTRACE2(atlTraceUtil, 0, "%p->Unmap()\n", surface);
      surface->Unmap();
    };

    size_t pixel_size = sd.Width * sd.Height;
    size_t frame_size = 4 * pixel_size;
    size_t data_size = sizeof(PackedVideoYuvFrame) + frame_size;
    ATLTRACE2(atlTraceUtil, 0, "Frame[%zu] %u * %u, %zu + %zu = %zu\n",
              CaptureYuv::GetInstance().GetFrameCount(), desc.Width,
              desc.Height, sizeof(PackedVideoYuvFrame), frame_size, data_size);
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
      if (DXGI_FORMAT_R8G8B8A8_UNORM == desc.Format) {
        ABGRToI420(mapped_rect.pBits, desc.Width * 4, y, desc.Width, u,
                   uv_stride, v, uv_stride, desc.Width, desc.Height);
      } else if (DXGI_FORMAT_B8G8R8A8_UNORM == desc.Format) {
        ARGBToI420(mapped_rect.pBits, desc.Width * 4, y, desc.Width, u,
                   uv_stride, v, uv_stride, desc.Width, desc.Height);
      } else {
        ATLTRACE2(atlTraceException, 0, "Unsupported format %u.", desc.Format);
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

  static void Free() { GetInstance().initialized_ = false; }

 private:
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
    return hr;
  }

 private:
  bool initialized_ = false;
  ID3D11Device* device_ = nullptr;
  ID3D11DeviceContext* context_ = nullptr;
  UINT width_ = 0;
  UINT height_ = 0;
  HWND window_ = nullptr;
};

class CaptureD3d11On12 {
 public:
  static CaptureD3d11On12& GetInstance() {
    static CaptureD3d11On12 instance;
    return instance;
  }

  static void Capture(void* swap, void*) {
    bool should_update = false;
    if (!GetInstance().initialized_) {
      HRESULT hr = GetInstance().Initialize(static_cast<IDXGISwapChain*>(swap));
      if (FAILED(hr)) {
        return;
      }
      GetInstance().initialized_ = true;
      should_update = true;
    }

    LARGE_INTEGER tick;
    QueryPerformanceCounter(&tick);

    size_t index = GetInstance().current_backbuffer_;
    if (GetInstance().is_dxgi_1_4_) {
      IDXGISwapChain3* swap3 = reinterpret_cast<IDXGISwapChain3*>(swap);
      index = swap3->GetCurrentBackBufferIndex();
      if (++GetInstance().current_backbuffer_ >=
          GetInstance().backbuffer_count_) {
        index = 0;
      }
    }

    ID3D11Resource* backbuffer = GetInstance().backbuffer11_[index];
    GetInstance().device11on12_->AcquireWrappedResources(&backbuffer, 1);

    D3D11_TEXTURE2D_DESC desc;
    CComPtr<ID3D11Texture2D> new_texture;
    HRESULT hr =
        CaptureTexture(GetInstance().device11_, GetInstance().context11_,
                       backbuffer, desc, new_texture);
    GetInstance().device11on12_->ReleaseWrappedResources(&backbuffer, 1);
    GetInstance().context11_->Flush();
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "!CaptureTexture(), #0x%08X\n", hr);
      return;
    }

    if (should_update) {
      SharedVideoFrameInfo* svfi =
          CaptureYuv::GetInstance().GetSharedVideoFrameInfo();
      svfi->timestamp = tick.QuadPart;
      svfi->type = VideoFrameType::kYuv;
      svfi->width = desc.Width;
      svfi->height = desc.Height;
      svfi->format = desc.Format;
      svfi->window = reinterpret_cast<std::uint64_t>(GetInstance().window_);
    }

    CComPtr<IDXGISurface> surface;
    hr = new_texture->QueryInterface(IID_PPV_ARGS(&surface));
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "!QueryInterface(IDXGISurface)\n");
      return;
    }

    VideoFrameStats stats = {};
    stats.timestamp = tick.QuadPart;
    stats.elapsed.preprocess = umu::TimeMeasure::Delta(tick.QuadPart);

    DXGI_SURFACE_DESC sd = {};
    hr = surface->GetDesc(&sd);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "%p->GetDesc() failed with 0x%08X.\n",
                surface, hr);
      return;
    }

    DXGI_MAPPED_RECT mapped_rect = {};
    {
      umu::TimeMeasure tm(stats.elapsed.rgb_mapping);
      hr = surface->Map(&mapped_rect, DXGI_MAP_READ);
    }
    ATLTRACE2(atlTraceUtil, 0, "frame[%zd] stats.elapsed.rgb_mapping = %llu.\n",
              CaptureYuv::GetInstance().GetFrameCount(),
              stats.elapsed.rgb_mapping);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "%p->Map() failed with 0x%08X.\n",
                surface, hr);
      if (DXGI_ERROR_DEVICE_REMOVED == hr || DXGI_ERROR_DEVICE_RESET == hr) {
        Free();
      }
      return;
    }
    BOOST_SCOPE_EXIT_ALL(&surface) {
      // ATLTRACE2(atlTraceUtil, 0, "%p->Unmap()\n", surface);
      surface->Unmap();
    };

    size_t pixel_size = sd.Width * sd.Height;
    size_t frame_size = 4 * pixel_size;
    size_t data_size = sizeof(PackedVideoYuvFrame) + frame_size;
    ATLTRACE2(atlTraceUtil, 0, "Frame[%zu] %u * %u, %zu + %zu = %zu\n",
              CaptureYuv::GetInstance().GetFrameCount(), desc.Width,
              desc.Height, sizeof(PackedVideoYuvFrame), frame_size, data_size);
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
      if (DXGI_FORMAT_R8G8B8A8_UNORM == desc.Format) {
        ABGRToI420(mapped_rect.pBits, mapped_rect.Pitch, y, desc.Width, u,
                   uv_stride, v, uv_stride, desc.Width, desc.Height);
      } else if (DXGI_FORMAT_B8G8R8A8_UNORM == desc.Format) {
        ARGBToI420(mapped_rect.pBits, mapped_rect.Pitch, y, desc.Width, u,
                   uv_stride, v, uv_stride, desc.Width, desc.Height);
      } else {
        ATLTRACE2(atlTraceException, 0, "Unsupported format %u.", desc.Format);
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

  static void Free() {
    GetInstance().initialized_ = false;
    GetInstance().FreeResource();
  }

 private:
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

    hr = D3D11On12CreateDevice_(device_, 0, nullptr, 0, nullptr, 0, 0,
                                &device11_, &context11_, nullptr);
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "!D3D11On12CreateDevice(), #0x%08X\n",
                hr);
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

  HRESULT InitFormat(IDXGISwapChain* swap) {
    CComPtr<IDXGISwapChain3> swap3;
    HRESULT hr = swap->QueryInterface(IID_PPV_ARGS(&swap3));
    if (SUCCEEDED(hr)) {
      is_dxgi_1_4_ = true;
      swap3.Release();
    } else {
      is_dxgi_1_4_ = false;
    }

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
    ATLTRACE2(atlTraceUtil, 0, "Format %u, %u * %u, Count = %u\n", format_,
              width_, height_, desc.SampleDesc.Count);

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

 private:
  static constexpr size_t kMaxBackbuffers = 8;

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
};

class CaptureDxgi {
 public:
  inline const IDXGISwapChain* GetSwapChain() const noexcept { return swap_; }

  inline void Reset() noexcept {
    swap_ = nullptr;
    capture_ = nullptr;
    free_ = nullptr;
  }

  bool Setup(IDXGISwapChain* swap) noexcept {
    CComPtr<ID3D11Device> d3d11;
    HRESULT hr11 = swap->GetDevice(IID_PPV_ARGS(&d3d11));
    if (SUCCEEDED(hr11)) {
      D3D_FEATURE_LEVEL level = d3d11->GetFeatureLevel();
      if (level >= D3D_FEATURE_LEVEL_11_0) {
        swap_ = swap;
        capture_ = CaptureD3d11::Capture;
        free_ = CaptureD3d11::Free;
        ATLTRACE2(atlTraceUtil, 0,
                  "%s: level(0x%x) >= D3D_FEATURE_LEVEL_11_0(0x%x)\n", __func__,
                  level, D3D_FEATURE_LEVEL_11_0);
        return true;
      }
      ATLTRACE2(atlTraceUtil, 0, "%s: level = 0x%x\n", __func__, level);
    }

    CComPtr<IUnknown> device;
    HRESULT hr = swap->GetDevice(__uuidof(ID3D10Device),
                                 reinterpret_cast<void**>(&device));
    if (SUCCEEDED(hr)) {
      swap_ = swap;
      ATLTRACE2(atlTraceUtil, 0, "%s: ID3D10Device\n", __func__);
      return true;
    }

    if (SUCCEEDED(hr11)) {
      swap_ = swap;
      capture_ = CaptureD3d11::Capture;
      free_ = CaptureD3d11::Free;
      ATLTRACE2(atlTraceUtil, 0, "%s: ID3D11Device\n", __func__);
      return true;
    }

    // d3d11on12
    hr = swap->GetDevice(__uuidof(ID3D12Device),
                         reinterpret_cast<void**>(&device));
    if (SUCCEEDED(hr)) {
      swap_ = swap;
      capture_ = CaptureD3d11On12::Capture;
      free_ = CaptureD3d11On12::Free;
      ATLTRACE2(atlTraceUtil, 0, "%s: ID3D12Device\n", __func__);
      return true;
    }

    return false;
  }

  inline bool CanCapture() const noexcept { return nullptr != capture_; }

  inline void Capture(IDXGISwapChain* swap) const noexcept {
    assert(nullptr != capture_);
    CComPtr<IDXGIResource> backbuffer;

    HRESULT hr = swap->GetBuffer(0, __uuidof(IUnknown),
                                 reinterpret_cast<void**>(&backbuffer));
    if (SUCCEEDED(hr)) {
      capture_(swap, backbuffer);
    } else {
      ATLTRACE2(atlTraceException, 0, "!GetBuffer(), #0x%08x\n", hr);
    }
  }

  inline void Free() const noexcept {
    if (nullptr != free_) {
      free_();
    }
  }

 private:
  IDXGISwapChain* swap_ = nullptr;
  void (*capture_)(void*, void*) = nullptr;
  void (*free_)() = nullptr;
};

CaptureDxgi capture;
}  // namespace

IDXGISWAPCHAIN_PRESENT HookDxgi::IDXGISwapChain_Present_ = nullptr;
IDXGISWAPCHAIN_RESIZEBUFFERS HookDxgi::IDXGISwapChain_ResizeBuffers_ = nullptr;
IDXGISWAPCHAIN1_PRESENT1 HookDxgi::IDXGISwapChain1_Present1_ = nullptr;
bool HookDxgi::resize_buffers_called_ = false;

bool HookDxgi::Hook() noexcept {
  HMODULE dxgi_module = GetModuleHandle(_T("dxgi.dll"));
  if (nullptr == dxgi_module) {
    ATLTRACE2(atlTraceUtil, 0, "dxgi.dll not loaded.\n");
    return false;
  }

  // d3d10.dll and d3d10_1.dll load d3d11.dll
  bool hooked = false;
  HMODULE d3d11_module = GetModuleHandle(_T("d3d11.dll"));
  if (nullptr != d3d11_module) {
    ATLTRACE2(atlTraceUtil, 0, "d3d11.dll loaded.\n");
    hooked = HookD3D(d3d11_module);
  } else if (nullptr != GetModuleHandle(_T("d3d12.dll"))) {
    ATLTRACE2(atlTraceUtil, 0, "d3d12.dll loaded.\n");
    loaded_d3d11_module_ = LoadLibrary(_T("d3d11.dll"));
    if (nullptr != loaded_d3d11_module_) {
      hooked = HookD3D(loaded_d3d11_module_);
    }
  }

  return hooked;
}

void HookDxgi::Unhook() noexcept {
  if (nullptr != loaded_d3d11_module_) {
    FreeLibrary(loaded_d3d11_module_);
    loaded_d3d11_module_ = nullptr;
  }
}

bool HookDxgi::HookD3D(HMODULE d3d11_module) noexcept {
  assert(nullptr != d3d11_module);

  const auto d3d11_create_device_and_swap_chain =
      reinterpret_cast<PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN>(
          GetProcAddress(d3d11_module, "D3D11CreateDeviceAndSwapChain"));
  if (nullptr == d3d11_create_device_and_swap_chain) {
    ATLTRACE2(atlTraceException, 0,
              "!GetProcAddress(D3D11CreateDeviceAndSwapChain), #%d\n",
              GetLastError());
    return false;
  }
  ATLTRACE2(atlTraceUtil, 0, "D3D11CreateDeviceAndSwapChain = 0x%p\n",
            d3d11_create_device_and_swap_chain);

  HWND hwnd = CreateWindowEx(0, L"Static", L"d3d11 temporary window", WS_POPUP,
                             0, 0, 2, 2, nullptr, nullptr, nullptr, nullptr);
  if (nullptr == hwnd) {
    ATLTRACE2(atlTraceException, 0, "!CreateWindowEx(), #%d\n", GetLastError());
    return false;
  }
  ATLTRACE2(atlTraceUtil, 0, "CreateWindowEx() = 0x%p\n", hwnd);
  BOOST_SCOPE_EXIT_ALL(&hwnd) { DestroyWindow(hwnd); };

  DXGI_SWAP_CHAIN_DESC desc = {};
  desc.BufferCount = 2;
  desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.BufferDesc.Width = 2;
  desc.BufferDesc.Height = 2;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.OutputWindow = hwnd;
  desc.SampleDesc.Count = 1;
  desc.Windowed = TRUE;

  CComPtr<IDXGISwapChain> swap_chain;
  CComPtr<ID3D11Device> device;
  D3D_FEATURE_LEVEL feature_levels[] = {
      D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0};
  D3D_FEATURE_LEVEL feature_level;
  HRESULT hr = d3d11_create_device_and_swap_chain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, feature_levels,
      _countof(feature_levels), D3D11_SDK_VERSION, &desc, &swap_chain, &device,
      &feature_level, nullptr);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "!D3D11CreateDeviceAndSwapChain(), #0x%08x\n", hr);
    return false;
  }
  ATLTRACE2(atlTraceUtil, 0, "FeatureLevel = 0x%04x\n", feature_level);

  // 8: swap_chain->Present
  IDXGISwapChain_Present_ = reinterpret_cast<IDXGISWAPCHAIN_PRESENT>(
      umu::memory::GetVTableFunctionAddress(swap_chain, 8));
  ATLTRACE2(atlTraceUtil, 0, "&IDXGISwapChain::Present = %p\n",
            IDXGISwapChain_Present_);

  // 13: swap_chain->ResizeBuffers
  IDXGISwapChain_ResizeBuffers_ =
      reinterpret_cast<IDXGISWAPCHAIN_RESIZEBUFFERS>(
          umu::memory::GetVTableFunctionAddress(swap_chain, 13));
  ATLTRACE2(atlTraceUtil, 0, "&IDXGISwapChain::ResizeBuffers = %p\n",
            IDXGISwapChain_ResizeBuffers_);

  IDXGISwapChain1* swap_chain1;
  hr = swap_chain->QueryInterface(IID_PPV_ARGS(&swap_chain1));
  if (SUCCEEDED(hr)) {
    // 22: swap_chain1->Present1
    IDXGISwapChain1_Present1_ = reinterpret_cast<IDXGISWAPCHAIN1_PRESENT1>(
        umu::memory::GetVTableFunctionAddress(swap_chain1, 22));
    ATLTRACE2(atlTraceUtil, 0, "&IDXGISwapChain1::Present1 = %p\n",
              IDXGISwapChain1_Present1_);
    swap_chain1->Release();
  }

  ATLTRACE2(atlTraceUtil, 0, "Begin hook d3d11...\n");

  NTSTATUS status = umu::HookAllThread(hook_IDXGISwapChain_Present_,
                                       IDXGISwapChain_Present_, MyPresent);
  if (!NT_SUCCESS(status)) {
    return false;
  }

  status = umu::HookAllThread(hook_IDXGISwapChain_ResizeBuffers_,
                              IDXGISwapChain_ResizeBuffers_, MyResizeBuffers);
  if (!NT_SUCCESS(status)) {
    return false;
  }

  if (nullptr != IDXGISwapChain1_Present1_) {
    status = umu::HookAllThread(hook_IDXGISwapChain1_Present1_,
                                IDXGISwapChain1_Present1_, MyPresent1);
    if (!NT_SUCCESS(status)) {
      return false;
    }
  }

  return true;
}

HRESULT STDMETHODCALLTYPE HookDxgi::MyPresent(IDXGISwapChain* swap,
                                              /* [in] */ UINT sync_interval,
                                              /* [in] */ UINT flags) {
  if (resize_buffers_called_) {
    resize_buffers_called_ = false;
    capture.Reset();
    ATLTRACE2(atlTraceUtil, 0, "%s(0x%p, %u, %u)\n", __func__, swap,
              sync_interval, flags);
  }

  if ((flags & DXGI_PRESENT_TEST) == 0) {
    if (nullptr == capture.GetSwapChain()) {
      capture.Setup(swap);
    } else if (swap != capture.GetSwapChain()) {
      capture.Free();
      capture.Setup(swap);
      ATLTRACE2(atlTraceUtil, 0, "%s: swap changed.\n", __func__);
    }

    if (capture.CanCapture()) {
      capture.Capture(swap);
    }
  }

  if (!CaptureYuv::GetInstance().IsPresentEnabled()) {
    flags |= DXGI_PRESENT_TEST;
  }
  return IDXGISwapChain_Present_(swap, sync_interval, flags);
}

HRESULT STDMETHODCALLTYPE
HookDxgi::MyResizeBuffers(IDXGISwapChain* This,
                          UINT BufferCount,
                          /* [in] */ UINT Width,
                          /* [in] */ UINT Height,
                          /* [in] */ DXGI_FORMAT NewFormat,
                          /* [in] */ UINT SwapChainFlags) {
  ATLTRACE2(atlTraceUtil, 0, "%s(0x%p, %u, %u, %u, %d, %u)\n", __func__, This,
            BufferCount, Width, Height, NewFormat, SwapChainFlags);

  capture.Free();

  HRESULT hr = IDXGISwapChain_ResizeBuffers_(This, BufferCount, Width, Height,
                                             NewFormat, SwapChainFlags);
  resize_buffers_called_ = true;
  return hr;
}

HRESULT STDMETHODCALLTYPE
HookDxgi::MyPresent1(IDXGISwapChain1* swap,
                     UINT sync_interval,
                     UINT flags,
                     _In_ const DXGI_PRESENT_PARAMETERS* present_parameters) {
  if (resize_buffers_called_) {
    resize_buffers_called_ = false;
    capture.Reset();
    ATLTRACE2(atlTraceUtil, 0, "%s(0x%p, %u, %u, 0x%p)\n", __func__, swap,
              sync_interval, flags, present_parameters);
  }

  if ((flags & DXGI_PRESENT_TEST) == 0) {
    capture.Setup(swap);

    if (capture.CanCapture()) {
      capture.Capture(swap);
    }
  }

  if (!CaptureYuv::GetInstance().IsPresentEnabled()) {
    flags |= DXGI_PRESENT_TEST;
  }
  return IDXGISwapChain1_Present1_(swap, sync_interval, flags,
                                   present_parameters);
}
