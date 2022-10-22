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

#include "captureyuv.h"
#include "d3d_utils.h"

#include "umu/time_measure.hpp"
#include "yuv/yuv.h"

namespace capture_d3d11 {

using namespace regame;

namespace {
bool initialized_ = false;
bool reset_ = false;
ID3D11Device* device_ = nullptr;
ID3D11DeviceContext* context_ = nullptr;
UINT width_ = 0;
UINT height_ = 0;
HWND window_ = nullptr;

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
  width_ = desc.BufferDesc.Width & ~1;
  height_ = desc.BufferDesc.Height & ~1;
  ATLTRACE2(atlTraceUtil, 0,
            "Format %u, %u * %u, Count %u, OutputWindow %p, Windowed %d\n",
            desc.BufferDesc.Format, desc.BufferDesc.Width,
            desc.BufferDesc.Height, desc.SampleDesc.Count, window_,
            desc.Windowed);
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
    width_ = desc.Width & ~1;
    height_ = desc.Height & ~1;

    SharedVideoFrameInfo* svfi = g_capture_yuv.GetSharedVideoFrameInfo();
    bool notify = false;
    if (reset_) {
      reset_ = false;
      if (svfi->width != width_ || svfi->height != height_) {
        notify = true;
      }
    }
    svfi->timestamp = tick.QuadPart;
    svfi->type = VideoFrameType::kI420;
    svfi->width = width_;
    svfi->height = height_;
    svfi->format = desc.Format;
    svfi->window = reinterpret_cast<std::uint64_t>(window_);
    if (notify) {
      g_capture_yuv.SetSharedFrameReadyEvent();
    }
  } else {
    UINT width = desc.Width & ~1;
    UINT height = desc.Height & ~1;
    if (width != width_ || height != height_) {
      width_ = width;
      height_ = height;

      SharedVideoFrameInfo* svfi = g_capture_yuv.GetSharedVideoFrameInfo();
      svfi->timestamp = tick.QuadPart;
      svfi->width = width_;
      svfi->height = height_;
      g_capture_yuv.SetSharedFrameReadyEvent();
    }
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

  // DXGI_SURFACE_DESC sd = {};
  // hr = surface->GetDesc(&sd);
  // if (FAILED(hr)) {
  //  ATLTRACE2(atlTraceException, 0, "%p->GetDesc() failed with 0x%08X.\n",
  //            surface, hr);
  //  return;
  //}

  DXGI_MAPPED_RECT mapped_rect = {};
  {
    umu::TimeMeasure tm(stats.elapsed.rgb_mapping);
    hr = surface->Map(&mapped_rect, DXGI_MAP_READ);
  }
  ATLTRACE2(atlTraceUtil, 0, "frame[%zd] stats.elapsed.rgb_mapping = %llu.\n",
            g_capture_yuv.GetFrameCount(), stats.elapsed.rgb_mapping);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "%p->Map() failed with 0x%08X.\n", surface,
              hr);
    if (DXGI_ERROR_DEVICE_REMOVED == hr || DXGI_ERROR_DEVICE_RESET == hr) {
      Free();
    }
    return;
  }
  BOOST_SCOPE_EXIT_ALL(&surface) {
    // ATLTRACE2(atlTraceUtil, 0, "%p->Unmap()\n", surface);
    surface->Unmap();
  };

  // assert(sd.Width == desc.Width);
  // assert(sd.Height == desc.Height);

  size_t pixel_size = width_ * height_;
  size_t frame_size = 4 * pixel_size;
  size_t data_size = sizeof(PackedVideoYuvFrame) + frame_size;
  ATLTRACE2(atlTraceUtil, 0, "Frame[%zu] %u * %u, %zu + %zu = %zu\n",
            g_capture_yuv.GetFrameCount(), desc.Width, desc.Height,
            sizeof(PackedVideoYuvFrame), frame_size, data_size);
  if (!g_capture_yuv.CreateSharedVideoYuvFrames(data_size)) {
    // unchanged but notify
    g_capture_yuv.SetSharedFrameReadyEvent();
    return;
  }

  auto frame = g_capture_yuv.GetAvailablePackedVideoYuvFrame();
  const int uv_stride = width_ >> 1;
  uint8_t* y = reinterpret_cast<uint8_t*>(frame->data);
  uint8_t* u = y + pixel_size;
  uint8_t* v = u + (pixel_size >> 2);

  {
    umu::TimeMeasure tm(stats.elapsed.yuv_convert);
    switch (desc.Format) {
      case DXGI_FORMAT_R8G8B8A8_UNORM:
        ABGRToI420(mapped_rect.pBits, mapped_rect.Pitch, y, width_, u,
                   uv_stride, v, uv_stride, width_, height_);
        break;
      case DXGI_FORMAT_B8G8R8A8_UNORM:
        ARGBToI420(mapped_rect.pBits, mapped_rect.Pitch, y, width_, u,
                   uv_stride, v, uv_stride, width_, height_);
        break;
      default:
        ATLTRACE2(atlTraceException, 0, "Unsupported format %u.", desc.Format);
        break;
    }
  }
  ATLTRACE2(atlTraceUtil, 0, "frame[%zd] stats.elapsed.yuv_convert = %llu\n",
            g_capture_yuv.GetFrameCount(), stats.elapsed.yuv_convert);

  stats.elapsed.total = umu::TimeMeasure::Delta(stats.timestamp);

  frame->stats.timestamp = stats.timestamp;
  frame->stats.elapsed.preprocess = stats.elapsed.preprocess;
  frame->stats.elapsed.nvenc = 0;
  frame->stats.elapsed.wait_rgb_mapping = stats.elapsed.wait_rgb_mapping;
  frame->stats.elapsed.rgb_mapping = stats.elapsed.rgb_mapping;
  frame->stats.elapsed.yuv_convert = stats.elapsed.yuv_convert;
  frame->stats.elapsed.total = stats.elapsed.total;

  g_capture_yuv.SetSharedFrameReadyEvent();
}

void Free() {
  initialized_ = false;
  reset_ = true;
}

}  // namespace capture_d3d11
