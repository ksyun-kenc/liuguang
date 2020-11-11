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

#include "process.h"

#include <Tlhelp32.h>

bool ProcessNamesToProcessIds(const std::vector<std::wstring>& process_names,
                              std::vector<DWORD>& pid_array) {
  HANDLE process_snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (INVALID_HANDLE_VALUE == process_snap) {
    return false;
  }

  bool result = false;
  PROCESSENTRY32 pe32 = {};
  pe32.dwSize = sizeof(PROCESSENTRY32);
  if (Process32First(process_snap, &pe32)) {
    do {
      if (pe32.th32ProcessID &&
          IsInArray(process_names, std::wstring(pe32.szExeFile))) {
        AddToArray(pid_array, pe32.th32ProcessID);
        result = true;
      }
    } while (Process32Next(process_snap, &pe32));
  }
  CloseHandle(process_snap);
  return result;
}
