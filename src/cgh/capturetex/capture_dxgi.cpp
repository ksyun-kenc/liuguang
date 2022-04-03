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

#include "capture_dxgi.h"

#include <d3d11on12.h>

#include "capture_dxgi_d3d11.h"
#include "capture_dxgi_d3d11on12.h"

bool CaptureDxgi::Setup(IDXGISwapChain* swap) noexcept {
  CComPtr<ID3D11Device> d3d11;
  HRESULT hr11 = swap->GetDevice(IID_PPV_ARGS(&d3d11));
  if (SUCCEEDED(hr11)) {
    D3D_FEATURE_LEVEL level = d3d11->GetFeatureLevel();
    if (level >= D3D_FEATURE_LEVEL_11_0) {
      swap_ = swap;
      capture_ = capture_d3d11::Capture;
      free_ = capture_d3d11::Free;
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
    capture_ = capture_d3d11::Capture;
    free_ = capture_d3d11::Free;
    ATLTRACE2(atlTraceUtil, 0, "%s: ID3D11Device\n", __func__);
    return true;
  }

  // d3d11on12
  hr = swap->GetDevice(__uuidof(ID3D12Device),
                       reinterpret_cast<void**>(&device));
  if (SUCCEEDED(hr)) {
    swap_ = swap;
    capture_ = capture_d3d11on12::Capture;
    free_ = capture_d3d11on12::Free;
    ATLTRACE2(atlTraceUtil, 0, "%s: ID3D12Device\n", __func__);
    return true;
  }

  return false;
}
