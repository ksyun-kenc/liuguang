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

#include "d3d9_capture.hpp"

#include "umu/memory.h"
#include "umu/scope_exit.hpp"

bool HookD3D9::Hook() {
  if (!InitializeApp()) {
    return false;
  }

  if (!InitializeD3D()) {
    return false;
  }

  NTSTATUS status =
      umu::HookAllThread(hook_IDirect3DSwapChain9_Present_,
                         IDirect3DSwapChain9_Present_, MySwapPresent);
  if (!NT_SUCCESS(status)) {
    ATLTRACE2(atlTraceException, 0,
              "hook IDirect3DSwapChain9::Present failed!\n");
    return false;
  }

  status = umu::HookAllThread(hook_IDirect3DDevice9Ex_Reset_,
                              IDirect3DDevice9Ex_Reset_, MyReset);
  if (!NT_SUCCESS(status)) {
    ATLTRACE2(atlTraceException, 0, "hook IDirect3DDevice9Ex::Reset failed!\n");
    return false;
  }

  status = umu::HookAllThread(hook_IDirect3DDevice9Ex_Present_,
                              IDirect3DDevice9Ex_Present_, MyPresent);
  if (!NT_SUCCESS(status)) {
    ATLTRACE2(atlTraceException, 0,
              "hook IDirect3DDevice9Ex::Present failed!\n");
    return false;
  }

  status = umu::HookAllThread(hook_IDirect3DDevice9Ex_PresentEx_,
                              IDirect3DDevice9Ex_PresentEx_, MyPresentEx);
  if (!NT_SUCCESS(status)) {
    ATLTRACE2(atlTraceException, 0,
              "hook IDirect3DDevice9Ex::PresentEx failed!\n");
    return false;
  }

  status = umu::HookAllThread(hook_IDirect3DDevice9Ex_ResetEx_,
                              IDirect3DDevice9Ex_ResetEx_, MyResetEx);
  if (!NT_SUCCESS(status)) {
    ATLTRACE2(atlTraceException, 0,
              "hook IDirect3DDevice9Ex::ResetEx failed!\n");
    return false;
  }

  return true;
}

void HookD3D9::Unhook() {
  D3D9Capture::GetInstance().Free();
  d3d9_module_.Free();
  if (nullptr != sa_.lpSecurityDescriptor) {
    LocalFree(sa_.lpSecurityDescriptor);
    sa_.lpSecurityDescriptor = nullptr;
  }
}

bool HookD3D9::InitializeApp() {
  if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
          _T("D:P(A;;GA;;;WD)"), SDDL_REVISION_1, &sa_.lpSecurityDescriptor,
          nullptr)) {
    return false;
  }

  return true;
}

bool HookD3D9::InitializeD3D() {
  DWORD error_code = d3d9_module_.GetOrLoad(L"d3d9.dll");
  if (NO_ERROR != error_code) {
    ATLTRACE2(atlTraceException, 0, "!Get(d3d9.dll), #%d\n", error_code);
    return false;
  }

  auto d3d9createex = reinterpret_cast<d3d9createex_t>(
      GetProcAddress(d3d9_module_, "Direct3DCreate9Ex"));
  if (nullptr == d3d9createex) {
    error_code = GetLastError();
    ATLTRACE2(atlTraceException, 0, "!Direct3DCreate9Ex, #%d\n", error_code);
    return false;
  }

  CComPtr<IDirect3D9Ex> d3d9ex;
  HRESULT hr = d3d9createex(D3D_SDK_VERSION, &d3d9ex);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "!Direct3DCreate9Ex(), #0x%08X\n", hr);
    return false;
  }

  HWND hwnd = CreateWindowEx(0, L"Static", L"d3d9 temporary window", WS_POPUP,
                             0, 0, 2, 2, nullptr, nullptr, nullptr, nullptr);
  if (nullptr == hwnd) {
    ATLTRACE2(atlTraceException, 0, "!CreateWindowEx(), #%d\n", GetLastError());
    return false;
  }
  ON_SCOPE_EXIT([&] { DestroyWindow(hwnd); });

  CComPtr<IDirect3DDevice9Ex> device;
  D3DPRESENT_PARAMETERS pp = {};
  pp.Windowed = true;
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
              "!IDirect3D9Ex::CreateDeviceEx(), #0x%08X\n", hr);
    return false;
  }

  CComPtr<IDirect3DSwapChain9> swap;
  hr = device->GetSwapChain(0, &swap);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "!IDirect3DDevice9Ex::GetSwapChain(), #0x%08X\n", hr);
    return false;
  }

  // swap->Present;
  IDirect3DSwapChain9_Present_ = reinterpret_cast<IDIRECT3DSWAPCHAIN9_PRESENT>(
      umu::memory::GetVTableFunctionAddress(swap, 3));
  ATLTRACE2(atlTraceUtil, 0, "&IDirect3DSwapChain9::Present = %p\n",
            IDirect3DSwapChain9_Present_);

  // 16: device->Reset;
  IDirect3DDevice9Ex_Reset_ = reinterpret_cast<IDIRECT3DDEVICE9EX_RESET>(
      umu::memory::GetVTableFunctionAddress(device, 16));
  ATLTRACE2(atlTraceUtil, 0, "&IDirect3DDevice9Ex::Reset = %p\n",
            IDirect3DDevice9Ex_Reset_);
  // 17: device->Present;
  IDirect3DDevice9Ex_Present_ = reinterpret_cast<IDIRECT3DDEVICE9EX_PRESENT>(
      umu::memory::GetVTableFunctionAddress(device, 17));
  ATLTRACE2(atlTraceUtil, 0, "&IDirect3DDevice9Ex::Present = %p\n",
            IDirect3DDevice9Ex_Present_);
  // 121: device->PresentEx;
  IDirect3DDevice9Ex_PresentEx_ =
      reinterpret_cast<IDIRECT3DDEVICE9EX_PRESENTEX>(
          umu::memory::GetVTableFunctionAddress(device, 121));
  ATLTRACE2(atlTraceUtil, 0, "&IDirect3DDevice9Ex::PresentEx = %p\n",
            IDirect3DDevice9Ex_PresentEx_);
  // 132: device->ResetEx;
  IDirect3DDevice9Ex_ResetEx_ = reinterpret_cast<IDIRECT3DDEVICE9EX_RESETEX>(
      umu::memory::GetVTableFunctionAddress(device, 132));
  ATLTRACE2(atlTraceUtil, 0, "&IDirect3DDevice9Ex::ResetEx = %p\n",
            IDirect3DDevice9Ex_ResetEx_);

  return true;
}

HRESULT STDMETHODCALLTYPE HookD3D9::MySwapPresent(IDirect3DSwapChain9* This,
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
    D3D9Capture::GetInstance().PresentBegin(device, &backbuffer);
  }

  hr = GetInstance().IDirect3DSwapChain9_Present_(
      This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

  if (nullptr != device) {
    D3D9Capture::GetInstance().PresentEnd(device, backbuffer);
  }

  return hr;
}

HRESULT STDMETHODCALLTYPE
HookD3D9::MyReset(IDirect3DDevice9Ex* This,
                  D3DPRESENT_PARAMETERS* pPresentationParameters) {
  D3D9Capture::GetInstance().Reset();
  return GetInstance().IDirect3DDevice9Ex_Reset_(This, pPresentationParameters);
}

// UMU: SDL calls IDirect3DDevice9Ex::Present
HRESULT STDMETHODCALLTYPE HookD3D9::MyPresent(IDirect3DDevice9Ex* This,
                                              CONST RECT* pSourceRect,
                                              CONST RECT* pDestRect,
                                              HWND hDestWindowOverride,
                                              CONST RGNDATA* pDirtyRegion) {
  HRESULT hr = S_OK;
  IDirect3DSurface9* backbuffer = nullptr;
  if (D3D9Capture::GetInstance().PresentBegin(This, &backbuffer)) {
    hr = GetInstance().IDirect3DDevice9Ex_Present_(
        This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
  }
  D3D9Capture::GetInstance().PresentEnd(This, backbuffer);
  return hr;
}

HRESULT STDMETHODCALLTYPE HookD3D9::MyPresentEx(IDirect3DDevice9Ex* This,
                                                CONST RECT* pSourceRect,
                                                CONST RECT* pDestRect,
                                                HWND hDestWindowOverride,
                                                CONST RGNDATA* pDirtyRegion,
                                                DWORD dwFlags) {
  HRESULT hr = S_OK;
  IDirect3DSurface9* backbuffer = nullptr;
  if (D3D9Capture::GetInstance().PresentBegin(This, &backbuffer)) {
    hr = GetInstance().IDirect3DDevice9Ex_PresentEx_(
        This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion,
        dwFlags);
  }
  D3D9Capture::GetInstance().PresentEnd(This, backbuffer);
  return hr;
}

HRESULT STDMETHODCALLTYPE
HookD3D9::MyResetEx(IDirect3DDevice9Ex* This,
                    D3DPRESENT_PARAMETERS* pPresentationParameters,
                    D3DDISPLAYMODEEX* pFullscreenDisplayMode) {
  D3D9Capture::GetInstance().Reset();
  return GetInstance().IDirect3DDevice9Ex_ResetEx_(
      This, pPresentationParameters, pFullscreenDisplayMode);
}
