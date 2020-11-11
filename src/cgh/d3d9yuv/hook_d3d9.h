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

#include "../EasyHook/Public/easyhook.h"
#include "umu/hook_api.hpp"
#include "umu/module.hpp"

typedef HRESULT(WINAPI* d3d9createex_t)(UINT, IDirect3D9Ex**);

typedef HRESULT(STDMETHODCALLTYPE* IDIRECT3DSWAPCHAIN9_PRESENT)(
    IDirect3DSwapChain9* This,
    CONST RECT* pSourceRect,
    CONST RECT* pDestRect,
    HWND hDestWindowOverride,
    CONST RGNDATA* pDirtyRegion,
    DWORD dwFlags);

typedef HRESULT(STDMETHODCALLTYPE* IDIRECT3DDEVICE9EX_RESET)(
    IDirect3DDevice9Ex* This,
    D3DPRESENT_PARAMETERS* pPresentationParameters);

typedef HRESULT(STDMETHODCALLTYPE* IDIRECT3DDEVICE9EX_PRESENT)(
    IDirect3DDevice9Ex* This,
    CONST RECT* pSourceRect,
    CONST RECT* pDestRect,
    HWND hDestWindowOverride,
    CONST RGNDATA* pDirtyRegion);

typedef HRESULT(STDMETHODCALLTYPE* IDIRECT3DDEVICE9EX_PRESENTEX)(
    IDirect3DDevice9Ex* This,
    CONST RECT* pSourceRect,
    CONST RECT* pDestRect,
    HWND hDestWindowOverride,
    CONST RGNDATA* pDirtyRegion,
    DWORD dwFlags);

typedef HRESULT(STDMETHODCALLTYPE* IDIRECT3DDEVICE9EX_RESETEX)(
    IDirect3DDevice9Ex* This,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode);

class HookD3D9 {
 public:
  static HookD3D9& GetInstance() {
    static HookD3D9 instance;
    return instance;
  }

  bool Hook();
  void Unhook();

  const SECURITY_ATTRIBUTES* GetSA() const noexcept { return &sa_; }
  SECURITY_ATTRIBUTES* SA() noexcept { return &sa_; }

 private:
  bool InitializeApp();
  bool InitializeD3D();

  static HRESULT STDMETHODCALLTYPE MySwapPresent(IDirect3DSwapChain9* This,
                                                 CONST RECT* pSourceRect,
                                                 CONST RECT* pDestRect,
                                                 HWND hDestWindowOverride,
                                                 CONST RGNDATA* pDirtyRegion,
                                                 DWORD dwFlags);

  static HRESULT STDMETHODCALLTYPE
  MyReset(IDirect3DDevice9Ex* This,
          D3DPRESENT_PARAMETERS* pPresentationParameters);

  static HRESULT STDMETHODCALLTYPE MyPresent(IDirect3DDevice9Ex* This,
                                             CONST RECT* pSourceRect,
                                             CONST RECT* pDestRect,
                                             HWND hDestWindowOverride,
                                             CONST RGNDATA* pDirtyRegion);

  static HRESULT STDMETHODCALLTYPE MyPresentEx(IDirect3DDevice9Ex* This,
                                               CONST RECT* pSourceRect,
                                               CONST RECT* pDestRect,
                                               HWND hDestWindowOverride,
                                               CONST RGNDATA* pDirtyRegion,
                                               DWORD dwFlags);

  static HRESULT STDMETHODCALLTYPE
  MyResetEx(IDirect3DDevice9Ex* This,
            D3DPRESENT_PARAMETERS* pPresentationParameters,
            D3DDISPLAYMODEEX* pFullscreenDisplayMode);

  umu::Module d3d9_module_;

  IDIRECT3DSWAPCHAIN9_PRESENT IDirect3DSwapChain9_Present_;
  IDIRECT3DDEVICE9EX_RESET IDirect3DDevice9Ex_Reset_;
  IDIRECT3DDEVICE9EX_PRESENT IDirect3DDevice9Ex_Present_;
  IDIRECT3DDEVICE9EX_PRESENTEX IDirect3DDevice9Ex_PresentEx_;
  IDIRECT3DDEVICE9EX_RESETEX IDirect3DDevice9Ex_ResetEx_;

  umu::HookApi hook_IDirect3DSwapChain9_Present_;
  umu::HookApi hook_IDirect3DDevice9Ex_Reset_;
  umu::HookApi hook_IDirect3DDevice9Ex_Present_;
  umu::HookApi hook_IDirect3DDevice9Ex_PresentEx_;
  umu::HookApi hook_IDirect3DDevice9Ex_ResetEx_;

  SECURITY_ATTRIBUTES sa_{};
};
