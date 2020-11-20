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

#include "cgvhid_client.h"

#include <windows.h>

#include <iostream>
#include <thread>

void TestKeyboard(CgvhidClient& cgvhid_client) {
  int ec = cgvhid_client.KeyboardReset();
  if (0 != ec) {
    std::cout << "KeyboardReset() = " << ec << '\n';
  }

  ec = cgvhid_client.KeyboardPress(VK_LWIN);
  if (0 != ec) {
    std::cout << "KeyboardPress() = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ec = cgvhid_client.KeyboardPress('R');
  if (0 != ec) {
    std::cout << "KeyboardPress() = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ec = cgvhid_client.KeyboardRelease('R');
  if (0 != ec) {
    std::cout << "KeyboardRelease() = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ec = cgvhid_client.KeyboardRelease(VK_LWIN);
  if (0 != ec) {
    std::cout << "KeyboardRelease() = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  for (int i = 'A'; i <= 'Z'; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ec = cgvhid_client.KeyboardPress(i);
    if (0 != ec) {
      std::cout << "KeyboardPress() = " << ec << '\n';
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ec = cgvhid_client.KeyboardRelease(i);
    if (0 != ec) {
      std::cout << "KeyboardRelease() = " << ec << '\n';
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  ec = cgvhid_client.KeyboardPress(VK_SHIFT);
  if (0 != ec) {
    std::cout << "KeyboardPress() = " << ec << '\n';
  }

  for (int i = 'A'; i <= 'Z'; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ec = cgvhid_client.KeyboardPress(i);
    if (0 != ec) {
      std::cout << "KeyboardPress() = " << ec << '\n';
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ec = cgvhid_client.KeyboardRelease(i);
    if (0 != ec) {
      std::cout << "KeyboardRelease() = " << ec << '\n';
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  ec = cgvhid_client.KeyboardRelease(VK_SHIFT);
  if (0 != ec) {
    std::cout << "KeyboardRelease() = " << ec << '\n';
  }

  ec = cgvhid_client.KeyboardReset();
  if (0 != ec) {
    std::cout << "KeyboardReset() = " << ec << '\n';
  }
}

int main() {
  CgvhidClient cgvhid_client;
  
  cgvhid_client.Init(3300, 2200);

  TestKeyboard(cgvhid_client);

  return 0;
}
