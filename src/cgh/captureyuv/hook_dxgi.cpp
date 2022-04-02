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

#include "capture_dxgi.h"
#include "captureyuv.h"

#include "umu/memory.h"
#include "umu/time_measure.hpp"

namespace {
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

  if (!g_capture_yuv.IsPresentEnabled()) {
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

  if (!g_capture_yuv.IsPresentEnabled()) {
    flags |= DXGI_PRESENT_TEST;
  }
  return IDXGISwapChain1_Present1_(swap, sync_interval, flags,
                                   present_parameters);
}
