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

#include <dxgi1_4.h>

#include "../EasyHook/Public/easyhook.h"

#include "umu/hook_api.hpp"

typedef HRESULT(STDMETHODCALLTYPE* IDXGISWAPCHAIN_PRESENT)(
    IDXGISwapChain* This,
    /* [in] */ UINT SyncInterval,
    /* [in] */ UINT PresentFlags);

typedef HRESULT(STDMETHODCALLTYPE* IDXGISWAPCHAIN_RESIZEBUFFERS)(
    IDXGISwapChain* This,
    /* [in] */ UINT BufferCount,
    /* [in] */ UINT Width,
    /* [in] */ UINT Height,
    /* [in] */ DXGI_FORMAT NewFormat,
    /* [in] */ UINT SwapChainFlags);

typedef HRESULT(STDMETHODCALLTYPE* IDXGISWAPCHAIN1_PRESENT1)(
    IDXGISwapChain1* This,
    /* [in] */ UINT SyncInterval,
    /* [in] */ UINT PresentFlags,
    /* [annotation][in] */
    _In_ const DXGI_PRESENT_PARAMETERS* pPresentParameters);

class HookDxgi {
 public:
  bool Hook() noexcept;
  void Unhook() noexcept;

 private:
  bool HookD3d11(HMODULE d3d11_module) noexcept;
  bool HookD3d12(HMODULE d3d12_module) noexcept;

 private:
  static HRESULT STDMETHODCALLTYPE MyPresent(IDXGISwapChain* This,
                                             UINT SyncInterval,
                                             UINT Flags);
  static HRESULT STDMETHODCALLTYPE MyResizeBuffers(IDXGISwapChain* This,
                                                   UINT BufferCount,
                                                   UINT Width,
                                                   UINT Height,
                                                   DXGI_FORMAT NewFormat,
                                                   UINT SwapChainFlags);
  static HRESULT STDMETHODCALLTYPE
  MyPresent1(IDXGISwapChain1* This,
             UINT SyncInterval,
             UINT Flags,
             _In_ const DXGI_PRESENT_PARAMETERS* pPresentParameters);

  static IDXGISWAPCHAIN_PRESENT IDXGISwapChain_Present_;
  static IDXGISWAPCHAIN_RESIZEBUFFERS IDXGISwapChain_ResizeBuffers_;
  static IDXGISWAPCHAIN1_PRESENT1 IDXGISwapChain1_Present1_;

  umu::HookApi hook_IDXGISwapChain_Present_;
  umu::HookApi hook_IDXGISwapChain_ResizeBuffers_;
  umu::HookApi hook_IDXGISwapChain1_Present1_;

  static bool resize_buffers_called_;
};
