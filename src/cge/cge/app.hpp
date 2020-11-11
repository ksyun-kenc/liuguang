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

#include <sddl.h>

class App {
 public:
  bool Init() noexcept {
    auto sec_desc = _T("D:P(A;;GA;;;WD)");
    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
            sec_desc, SDDL_REVISION_1, &sa_.lpSecurityDescriptor, nullptr)) {
      return false;
    }
    QueryPerformanceFrequency(&frequency_);
    return true;
  }

  ~App() {
    if (nullptr != sa_.lpSecurityDescriptor) {
      LocalFree(sa_.lpSecurityDescriptor);
    }
  }

  LARGE_INTEGER GetFrequency() const noexcept { return frequency_; }
  const SECURITY_ATTRIBUTES* GetSA() const noexcept { return &sa_; }
  SECURITY_ATTRIBUTES* SA() noexcept { return &sa_; }

 private:
  LARGE_INTEGER frequency_{};
  SECURITY_ATTRIBUTES sa_{};
};
