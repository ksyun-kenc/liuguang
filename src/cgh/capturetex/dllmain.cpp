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

#include "capturetex.h"

#ifdef CAPTURETEX_EXPORTS
#define CAPTURETEX_API __declspec(dllexport)
#else
#define CAPTURETEX_API __declspec(dllimport)
#endif

CaptureTex g_capture_tex;

BOOL APIENTRY DllMain(HMODULE instance,
                      DWORD reason_for_call,
                      LPVOID reserved) {
  switch (reason_for_call) {
    case DLL_PROCESS_ATTACH:
      ATLTRACE2(atlTraceUtil, 0, "%s: DLL_PROCESS_ATTACH\n", __func__);
      break;
    // case DLL_THREAD_ATTACH:
    //  // may crash
    //  ATLTRACE2(atlTraceUtil, 0, "%s: DLL_THREAD_ATTACH\n", __func__);
    //  break;
    // case DLL_THREAD_DETACH:
    //  ATLTRACE2(atlTraceUtil, 0, "%s: DLL_THREAD_DETACH\n", __func__);
    //  break;
    case DLL_PROCESS_DETACH:
      ATLTRACE2(atlTraceUtil, 0, "%s: DLL_PROCESS_DETACH\n", __func__);
      // HookThread already killed when reach here
      g_capture_tex.Free();
      break;
  }
  return TRUE;
}

extern "C" CAPTURETEX_API void __stdcall NativeInjectionEntryPoint(
    REMOTE_ENTRY_INFO* remote_info) {
  ATLTRACE2(atlTraceUtil, 0, "%s: +\n", __func__);
  if (!g_capture_tex.Run()) {
    ATLTRACE2(atlTraceException, 0, "%s: failed!\n", __func__);
  }
  RhWakeUpProcess();
}
