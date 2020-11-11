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

#include <windows.h>

#include <string>
#include <vector>

template <typename T>
bool AddToArray(std::vector<T>& _array, T& item) {
  for (const auto& e : _array) {
    if (e == item) {
      return false;
    }
  }
  //_array.push_back(item);
  _array.emplace_back(item);
  return true;
}

inline bool IsInArray(const std::vector<std::wstring>& _array,
                      const std::wstring& item) {
  for (auto& e : _array) {
    if (0 == _wcsicmp(e.data(), item.data())) {
      return true;
    }
  }
  return false;
}

bool ProcessNamesToProcessIds(const std::vector<std::wstring>& process_names,
                              std::vector<DWORD>& pid_array);
