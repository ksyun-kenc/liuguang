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

#include <mutex>

#include "hook_d3d9.h"

#pragma comment(lib, "yuv.lib")

#ifdef D3D9YUV_EXPORTS
#define HOOK_API __declspec(dllexport)
#else
#define HOOK_API __declspec(dllimport)
#endif

BOOL APIENTRY DllMain(HMODULE module, DWORD reason_for_call, LPVOID reserved) {
  switch (reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      HookD3D9::GetInstance().Unhook();
      break;
  }
  return TRUE;
}

// std::once_flag hooked;

extern "C" HOOK_API void __stdcall NativeInjectionEntryPoint(
    REMOTE_ENTRY_INFO* remote_info) {
  HookD3D9::GetInstance().Unhook();
  HookD3D9::GetInstance().Hook();
  // std::call_once(hooked, []() { HookD3D9::GetInstance().Hook(); });
  RhWakeUpProcess();
}
