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

#include <d3d11.h>

class CaptureDxgi {
 public:
  const IDXGISwapChain* GetSwapChain() const noexcept { return swap_; }

  void Reset() noexcept {
    swap_ = nullptr;
    capture_ = nullptr;
    if (nullptr != free_) {
      free_();
      free_ = nullptr;
    }
  }

  bool Setup(IDXGISwapChain* swap) noexcept;

  bool CanCapture() const noexcept { return nullptr != capture_; }

  void Capture(IDXGISwapChain* swap) const noexcept {
    assert(nullptr != capture_);
    CComPtr<IDXGIResource> backbuffer;

    HRESULT hr = swap->GetBuffer(0, __uuidof(IUnknown),
                                 reinterpret_cast<void**>(&backbuffer));
    if (SUCCEEDED(hr)) {
      capture_(swap, backbuffer);
    } else {
      ATLTRACE2(atlTraceException, 0, "!GetBuffer(), #0x%08x\n", hr);
    }
  }

  void Free() const noexcept {
    if (nullptr != free_) {
      free_();
    }
  }

 private:
  IDXGISwapChain* swap_ = nullptr;
  void (*capture_)(void*, void*) = nullptr;
  void (*free_)() = nullptr;
};
