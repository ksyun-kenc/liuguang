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

#include "capture_d3d9.h"

#include "captureyuv.h"

#include "umu/apppath_t.h"
#include "umu/time_measure.hpp"
#include "yuv/yuv.h"

using namespace regame;

void CaptureD3d9::Capture(IDirect3DDevice9* device,
                          IDirect3DSurface9* backbuffer) noexcept {
  if (!g_capture_yuv.IsEncoderStarted()) {
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
  if (D3DERR_INVALIDCALL == hr) {
    // When use MSAA may return D3DERR_INVALIDCALL(0x8876086C)
    ATLTRACE2(atlTraceException, 0,
              "%s: !GetRenderTargetData(), D3DERR_INVALIDCALL\n", __func__, hr);
    Reset();
    return;
  } else if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "%s: !GetRenderTargetData(), #0x%08X\n",
              __func__, hr);
    Reset();
    return;
  }

  D3DSURFACE_DESC desc = {};
  hr = copy_surface_->GetDesc(&desc);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "%s: %p->GetDesc() failed with 0x%08X.\n",
              __func__, copy_surface_, hr);
    return;
  }
  if (desc.Type != D3DRTYPE_SURFACE && desc.Type != D3DRTYPE_TEXTURE) {
    ATLTRACE2(atlTraceException, 0, "%s: %p->Desc.Type = %d.\n", __func__,
              copy_surface_, desc.Type);
    return;
  }
  if (desc.MultiSampleType != D3DMULTISAMPLE_NONE) {
    ATLTRACE2(atlTraceException, 0, "%s: %p->Desc.MultiSampleType = %d.\n",
              __func__, copy_surface_, desc.MultiSampleType);
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
    svfi->type = VideoFrameType::kYuv;
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

  VideoFrameStats stats = {};
  stats.timestamp = tick.QuadPart;
  stats.elapsed.preprocess = umu::TimeMeasure::Delta(tick.QuadPart);
  ATLTRACE2(atlTraceUtil, 0, "%s: stats.elapsed.prepare = %llu.\n", __func__,
            stats.elapsed.preprocess);

  // wait_rgb_mapping = 0
  D3DLOCKED_RECT mapped_rect = {};
  {
    umu::TimeMeasure tm(stats.elapsed.rgb_mapping);
    // LockRect may spend long time
    // ATLTRACE2(atlTraceUtil, 0, "%s: LockRect +\n", __func__);
    hr = copy_surface_->LockRect(&mapped_rect, nullptr, D3DLOCK_READONLY);
    // ATLTRACE2(atlTraceUtil, 0, "%s: LockRect -, 0x%08X\n", __func__, hr);
  }
  ATLTRACE2(atlTraceUtil, 0, "frame[%zd] stats.elapsed.rgb_mapping = %llu.\n",
            g_capture_yuv.GetFrameCount(), stats.elapsed.rgb_mapping);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "%s: %p->LockRect() failed with 0x%08X.\n",
              __func__, copy_surface_, hr);
    return;
  }
  BOOST_SCOPE_EXIT_ALL(this) { copy_surface_->UnlockRect(); };

  size_t pixel_size = width_ * height_;
  size_t frame_size = 4 * pixel_size;
  size_t data_size = sizeof(PackedVideoYuvFrame) + frame_size;
  if (!g_capture_yuv.CreateSharedVideoYuvFrames(data_size)) {
    // unchanged but notify
    g_capture_yuv.SetSharedFrameReadyEvent();
    return;
  }

  auto frame = g_capture_yuv.GetAvailablePackedVideoYuvFrame();
  const int uv_stride = width_ >> 1;
  uint8_t* y = reinterpret_cast<uint8_t*>(frame->data);
  uint8_t* u = y + pixel_size;
  uint8_t* v = u + (pixel_size / 4);

  {
    umu::TimeMeasure tm(stats.elapsed.yuv_convert);
    switch (desc.Format) {
      case D3DFMT_A8R8G8B8:
      case D3DFMT_X8R8G8B8:
        ARGBToI420(static_cast<uint8_t*>(mapped_rect.pBits), mapped_rect.Pitch,
                   y, width_, u, uv_stride, v, uv_stride, width_, height_);
        break;
      default:
        ATLTRACE2(atlTraceUtil, 0, "%s: Format = %d\n", __func__, desc.Format);
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

HRESULT CaptureD3d9::GetBackbuffer(IDirect3DDevice9* device,
                                   IDirect3DSurface9** surface) {
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

bool CaptureD3d9::InitFormatBackbuffer() noexcept {
  D3DPRESENT_PARAMETERS pp;

  if (!GetSwapDesc(&pp)) {
    return false;
  }

  window_ = pp.hDeviceWindow;

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
      width_ = desc.Width & ~1;
      height_ = desc.Height & ~1;

      return true;
    }
  }

  d3d9_format_ = pp.BackBufferFormat;
  width_ = pp.BackBufferWidth & ~1;
  height_ = pp.BackBufferHeight & ~1;
  return true;
}
