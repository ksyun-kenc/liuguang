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

#include "sdl_hack.h"

#include <atlstr.h>
#include <sddl.h>

#include <boost/scope_exit.hpp>

#include "sdl_internal.h"

//#include "umu/apppath_t.h"
#include "umu/com_initializer.hpp"
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

HRESULT CaptureTexture(_In_ ID3D11DeviceContext* context,
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

  CComPtr<ID3D11Device> device;
  context->GetDevice(&device);

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
}  // namespace

SdlHack::~SdlHack() {
  if (nullptr != sa_.lpSecurityDescriptor) {
    LocalFree(sa_.lpSecurityDescriptor);
  }
  SetEvent(stop_event_);
  if (waiting_thread_.joinable()) {
    waiting_thread_.join();
  }
}

bool SdlHack::Init() noexcept {
  SDL_version compiled;
  SDL_VERSION(&compiled);
  if (2 == compiled.major && 0 == compiled.minor && 12 <= compiled.patch) {
    SDL_GetVersion(&linked_);
    if (2 == linked_.major && 0 == linked_.minor && 12 == linked_.patch) {
      // yes
    } else if (2 == linked_.major && 0 == linked_.minor &&
               14 == linked_.patch) {
      // yes
    } else if (2 == linked_.major && 0 == linked_.minor &&
               16 == linked_.patch) {
      // yes
    } else {
      CString error_text;
      error_text.Format(
          _T("Compiled with SDL %u.%u.%u\nLinked to SDL %u.%u.%u"),
          compiled.major, compiled.minor, compiled.patch, linked_.major,
          linked_.minor, linked_.patch);
      MessageBox(nullptr, error_text, _T("Warning"), MB_ICONWARNING);
    }
  } else {
    MessageBox(nullptr, _T("sdl_internal.h is only for SDL 2.0.12+!"), nullptr,
               MB_ICONERROR);
  }

  auto sec_desc = _T("D:P(A;;GA;;;WD)");
  if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
          sec_desc, SDDL_REVISION_1, &sa_.lpSecurityDescriptor, nullptr)) {
    return false;
  }

  HANDLE ev = CreateEvent(&sa_, TRUE, FALSE, kVideoStartedEventName.data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return false;
  }
  encoder_started_event_.Attach(ev);

  ev = CreateEvent(&sa_, TRUE, FALSE, kVideoStoppedEventName.data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return false;
  }
  encoder_stopped_event_.Attach(ev);

  ev = CreateEvent(&sa_, FALSE, FALSE, kSharedVideoFrameReadyEventName.data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return false;
  }
  shared_frame_ready_event_.Attach(ev);

  HRESULT hr = shared_frame_info_.MapSharedMem(
      sizeof(SharedVideoFrameInfo), kSharedVideoFrameInfoFileMappingName.data(),
      nullptr, &sa_);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "MapSharedMem() failed with 0x%08x.\n", hr);
    return false;
  }

  ev = CreateEvent(&sa_, TRUE, FALSE, nullptr);
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return false;
  }
  stop_event_.Attach(ev);

  waiting_thread_ = std::thread(&SdlHack::WaitingThread, this);
  if (nullptr == waiting_thread_.native_handle()) {
    ATLTRACE2(atlTraceException, 0, "Create thread failed with %u",
              GetLastError());
    return false;
  }
  return true;
}

bool SdlHack::IsStarted() const noexcept {
  return is_started_;
}

void SdlHack::GetTexture(SDL_Renderer* renderer) {
  D3D11_RenderData* render_data = nullptr;
  if (2 == linked_.major && 0 == linked_.minor && 12 == linked_.patch) {
    render_data = static_cast<D3D11_RenderData*>(
        reinterpret_cast<SDL_Renderer_2_0_12*>(renderer)->driverdata);
  } else if (2 == linked_.major && 0 == linked_.minor && 14 == linked_.patch) {
    render_data = static_cast<D3D11_RenderData*>(
        reinterpret_cast<SDL_Renderer_2_0_14*>(renderer)->driverdata);
  } else if (2 == linked_.major && 0 == linked_.minor && 16 == linked_.patch) {
    render_data = static_cast<D3D11_RenderData*>(
        reinterpret_cast<SDL_Renderer_2_0_16*>(renderer)->driverdata);
  } else {
    return;
  }
  if (nullptr == render_data) {
    ATLTRACE2(atlTraceException, 0, "!render_data\n");
    return;
  }

  LARGE_INTEGER tick;
  QueryPerformanceCounter(&tick);

  bool should_update = false;
  if (nullptr == swap_) {
    should_update = true;
    swap_ = render_data->swapChain;
  } else if (swap_ != render_data->swapChain) {
    should_update = true;
    swap_ = render_data->swapChain;
  }

  CComPtr<IDXGIResource> backbuffer;
  HRESULT hr = render_data->swapChain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "!GetBuffer()\n");
    return;
  }

  CComQIPtr<ID3D11Texture2D> acquired_texture(backbuffer);
  if (!acquired_texture) {
    ATLTRACE2(atlTraceException, 0, "!QueryInterface(ID3D11Texture2D)\n");
    return;
  }

  D3D11_TEXTURE2D_DESC desc;
  CComPtr<ID3D11Texture2D> new_texture;
  hr = CaptureTexture(render_data->d3dContext, acquired_texture, desc,
                      new_texture);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "!CaptureTexture(), #0x%08X\n", hr);
    return;
  }

  if (should_update) {
    SharedVideoFrameInfo* svfi = shared_frame_info_;
    svfi->timestamp = tick.QuadPart;
    svfi->type = VideoFrameType::YUV;
    svfi->width = desc.Width;
    svfi->height = desc.Height;
    svfi->format = desc.Format;
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
            frame_count_, stats.elapsed.rgb_mapping);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "%p->Map() failed with 0x%08X.\n", surface,
              hr);
    if (DXGI_ERROR_DEVICE_REMOVED == hr || DXGI_ERROR_DEVICE_RESET == hr) {
      SetEvent(stop_event_);
    }
    return;
  }
  BOOST_SCOPE_EXIT_ALL(&) {
    ATLTRACE2(atlTraceUtil, 0, "%p->Unmap()\n", surface);
    surface->Unmap();
  };

  size_t pixel_size = sd.Width * sd.Height;
  size_t frame_size = 4 * pixel_size;
  size_t data_size = sizeof(PackedVideoYuvFrame) + frame_size;
  ATLTRACE2(atlTraceUtil, 0, "Frame[%zu] %u * %u, %zu + %zu = %zu\n",
            frame_count_, desc.Width, desc.Height, sizeof(PackedVideoYuvFrame),
            frame_size, data_size);

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
        kSharedVideoYuvFramesFileMappingName.data(), nullptr, &sa_);
    if (FAILED(hr)) {
      SetEvent(stop_event_);
      ATLTRACE2(atlTraceException, 0, "MapSharedMem() failed.\n");
      return;
    }

    ATLTRACE2(atlTraceUtil, 0, "MapSharedMem size = %zu + %zu * 2 = %zu\n",
              sizeof(SharedVideoYuvFrames), data_size,
              sizeof(SharedVideoYuvFrames) + data_size * kNumberOfSharedFrames);

    auto frames = static_cast<SharedVideoYuvFrames*>(shared_frames_.GetData());
    frames->data_size = static_cast<uint32_t>(data_size);
  }

  auto frame = reinterpret_cast<PackedVideoYuvFrame*>(
      static_cast<char*>(shared_frames_) + sizeof(SharedVideoYuvFrames) +
      (frame_count_ % kNumberOfSharedFrames) * data_size);
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
  ATLTRACE2(atlTraceUtil, 0, "frame[%zd] stats.elapsed.yuv_convert = %llu, \n",
            frame_count_, stats.elapsed.yuv_convert);

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

int SdlHack::WaitingThread() {
  for (;;) {
    HANDLE started_events[] = {stop_event_, encoder_started_event_};
    DWORD wait = WaitForMultipleObjects(_countof(started_events),
                                        started_events, FALSE, INFINITE);
    if (WAIT_OBJECT_0 == wait) {
      ATLTRACE2(atlTraceUtil, 0, "%s: stopping.\n", __func__);
      break;
    } else if (WAIT_OBJECT_0 + 1 == wait) {
      is_started_ = true;
      ATLTRACE2(atlTraceUtil, 0, "%s: Video encoder is started.\n", __func__);
    } else {
      int error_code = GetLastError();
      ATLTRACE2(atlTraceException, 0,
                "%s: WaitForMultipleObjects() return %u, error %u.\n", __func__,
                wait, error_code);
      return error_code;
    }

    HANDLE stop_events[] = {stop_event_, encoder_stopped_event_};
    wait = WaitForMultipleObjects(_countof(stop_events), stop_events, FALSE,
                                  INFINITE);
    if (WAIT_OBJECT_0 == wait) {
      ATLTRACE2(atlTraceUtil, 0, "%s: stopping.\n", __func__);
      break;
    } else if (WAIT_OBJECT_0 + 1 == wait) {
      is_started_ = false;
      ATLTRACE2(atlTraceUtil, 0, "%s: Video encoder is stopped.\n", __func__);
    } else {
      int error_code = GetLastError();
      ATLTRACE2(atlTraceException, 0,
                "%s: WaitForMultipleObjects() return %u, error %u.\n", __func__,
                wait, error_code);
      return error_code;
    }
  }
  return 0;
}
