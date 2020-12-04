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

typedef HRESULT(STDMETHODCALLTYPE* IDIRECT3DSWAPCHAIN9_PRESENT)(
    IDirect3DSwapChain9* This,
    CONST RECT* pSourceRect,
    CONST RECT* pDestRect,
    HWND hDestWindowOverride,
    CONST RGNDATA* pDirtyRegion,
    DWORD dwFlags);

typedef ULONG(STDMETHODCALLTYPE* IDIRECT3DDEVICE9EX_RELEASE)(
    IDirect3DDevice9Ex* This);

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

class HookD3d9 {
 public:
  bool Hook() noexcept;
  void Unhook() noexcept;

 private:
  static ULONG STDMETHODCALLTYPE MyRelease(IDirect3DDevice9Ex* This);

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

  static IDIRECT3DDEVICE9EX_RELEASE IDirect3DDevice9Ex_Release_;

  static IDIRECT3DSWAPCHAIN9_PRESENT IDirect3DSwapChain9_Present_;
  static IDIRECT3DDEVICE9EX_RESET IDirect3DDevice9Ex_Reset_;
  static IDIRECT3DDEVICE9EX_PRESENT IDirect3DDevice9Ex_Present_;
  static IDIRECT3DDEVICE9EX_PRESENTEX IDirect3DDevice9Ex_PresentEx_;
  static IDIRECT3DDEVICE9EX_RESETEX IDirect3DDevice9Ex_ResetEx_;

  umu::HookApi hook_IDirect3DDevice9Ex_Release_;

  umu::HookApi hook_IDirect3DSwapChain9_Present_;
  umu::HookApi hook_IDirect3DDevice9Ex_Reset_;
  umu::HookApi hook_IDirect3DDevice9Ex_Present_;
  umu::HookApi hook_IDirect3DDevice9Ex_PresentEx_;
  umu::HookApi hook_IDirect3DDevice9Ex_ResetEx_;
};
