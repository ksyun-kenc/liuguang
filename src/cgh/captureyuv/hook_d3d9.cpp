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

#include "capture_d3d9.h"
#include "captureyuv.h"

#include "umu/memory.h"

namespace {
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
  BOOST_SCOPE_EXIT_ALL(&hwnd) { DestroyWindow(hwnd); };

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

  if (g_capture_yuv.IsPresentEnabled()) {
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
  if (g_capture_yuv.IsPresentEnabled()) {
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
  if (g_capture_yuv.IsPresentEnabled()) {
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
