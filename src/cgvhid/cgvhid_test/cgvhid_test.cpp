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

  ec = cgvhid_client.KeyboardVkPress(VK_LWIN);
  if (0 != ec) {
    std::cout << "KeyboardVkPress() = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ec = cgvhid_client.KeyboardVkPress('R');
  if (0 != ec) {
    std::cout << "KeyboardVkPress() = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ec = cgvhid_client.KeyboardVkRelease('R');
  if (0 != ec) {
    std::cout << "KeyboardVkRelease() = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  ec = cgvhid_client.KeyboardVkRelease(VK_LWIN);
  if (0 != ec) {
    std::cout << "KeyboardVkRelease() = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  for (int i = 'A'; i <= 'Z'; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ec = cgvhid_client.KeyboardVkPress(i);
    if (0 != ec) {
      std::cout << "KeyboardVkPress() = " << ec << '\n';
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ec = cgvhid_client.KeyboardVkRelease(i);
    if (0 != ec) {
      std::cout << "KeyboardVkRelease() = " << ec << '\n';
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  ec = cgvhid_client.KeyboardVkPress(VK_LSHIFT);
  if (0 != ec) {
    std::cout << "KeyboardVkPress() = " << ec << '\n';
  }

  for (int i = 'A'; i <= 'Z'; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ec = cgvhid_client.KeyboardVkPress(i);
    if (0 != ec) {
      std::cout << "KeyboardVkPress() = " << ec << '\n';
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ec = cgvhid_client.KeyboardVkRelease(i);
    if (0 != ec) {
      std::cout << "KeyboardVkRelease() = " << ec << '\n';
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  ec = cgvhid_client.KeyboardVkRelease(VK_LSHIFT);
  if (0 != ec) {
    std::cout << "KeyboardVkRelease() = " << ec << '\n';
  }

  ec = cgvhid_client.KeyboardReset();
  if (0 != ec) {
    std::cout << "KeyboardReset() = " << ec << '\n';
  }
}

void TestMouse(CgvhidClient& cgvhid_client) {
  int ec = cgvhid_client.MouseReset();
  if (0 != ec) {
    std::cout << "MouseReset() = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ec = cgvhid_client.AbsoluteMouseMove(1000, 1000);
  if (0 != ec) {
    std::cout << "AbsoluteMouseMove() = " << ec << '\n';
  }

  std::cout << "Calling AbsoluteMouseWheel()\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  ec = cgvhid_client.AbsoluteMouseWheel(4, 0);
  if (0 != ec) {
    std::cout << "AbsoluteMouseWheel(4, 0) = " << ec << '\n';
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  ec = cgvhid_client.AbsoluteMouseWheel(-4, 0);
  if (0 != ec) {
    std::cout << "AbsoluteMouseWheel(-4, 0) = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  ec = cgvhid_client.RelativeMouseMove(100, 0);
  if (0 != ec) {
    std::cout << "RelativeMouseMove() = " << ec << '\n';
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  ec = cgvhid_client.RelativeMouseMove(0, 100);
  if (0 != ec) {
    std::cout << "RelativeMouseMove() = " << ec << '\n';
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  ec = cgvhid_client.RelativeMouseMove(-100, 0);
  if (0 != ec) {
    std::cout << "RelativeMouseMove() = " << ec << '\n';
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  ec = cgvhid_client.RelativeMouseMove(0, -100);
  if (0 != ec) {
    std::cout << "RelativeMouseMove() = " << ec << '\n';
  }

  std::cout << "Calling RelativeMouseWheel()\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  ec = cgvhid_client.RelativeMouseWheel(4, 0);
  if (0 != ec) {
    std::cout << "RelativeMouseWheel(4, 0) = " << ec << '\n';
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  ec = cgvhid_client.RelativeMouseWheel(-4, 0);
  if (0 != ec) {
    std::cout << "RelativeMouseWheel(-4, 0) = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  ec = cgvhid_client.RelativeMouseWheel(0, 1);
  if (0 != ec) {
    std::cout << "RelativeMouseWheel(0, 1) = " << ec << '\n';
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  ec = cgvhid_client.RelativeMouseWheel(0, -1);
  if (0 != ec) {
    std::cout << "RelativeMouseWheel(0, -1) = " << ec << '\n';
  }

#if MOUSE_BUTTON
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  ec = cgvhid_client.RelativeMouseButtonPress(
      CgvhidMouseButton::kCgvhidMouseButtonLeft);
  if (0 != ec) {
    std::cout << "RelativeMouseButtonPress(Left) = " << ec << '\n';
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ec = cgvhid_client.RelativeMouseButtonRelease(
      CgvhidMouseButton::kCgvhidMouseButtonLeft);
  if (0 != ec) {
    std::cout << "RelativeMouseButtonRelease(Left) = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  ec = cgvhid_client.RelativeMouseButtonPress(
      CgvhidMouseButton::kCgvhidMouseButtonMiddle);
  if (0 != ec) {
    std::cout << "RelativeMouseButtonPress(Middle) = " << ec << '\n';
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ec = cgvhid_client.RelativeMouseButtonRelease(
      CgvhidMouseButton::kCgvhidMouseButtonMiddle);
  if (0 != ec) {
    std::cout << "RelativeMouseButtonRelease(Middle) = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  ec = cgvhid_client.RelativeMouseButtonPress(
      CgvhidMouseButton::kCgvhidMouseButtonRight);
  if (0 != ec) {
    std::cout << "RelativeMouseButtonPress(Right) = " << ec << '\n';
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ec = cgvhid_client.RelativeMouseButtonRelease(
      CgvhidMouseButton::kCgvhidMouseButtonRight);
  if (0 != ec) {
    std::cout << "RelativeMouseButtonRelease(Right) = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  ec = cgvhid_client.RelativeMouseButtonPress(
      CgvhidMouseButton::kCgvhidMouseButtonX1);
  if (0 != ec) {
    std::cout << "RelativeMouseButtonPress(X1) = " << ec << '\n';
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ec = cgvhid_client.RelativeMouseButtonRelease(
      CgvhidMouseButton::kCgvhidMouseButtonX1);
  if (0 != ec) {
    std::cout << "RelativeMouseButtonRelease(X1) = " << ec << '\n';
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  ec = cgvhid_client.RelativeMouseButtonPress(
      CgvhidMouseButton::kCgvhidMouseButtonX2);
  if (0 != ec) {
    std::cout << "RelativeMouseButtonPress(X2) = " << ec << '\n';
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ec = cgvhid_client.RelativeMouseButtonRelease(
      CgvhidMouseButton::kCgvhidMouseButtonX2);
  if (0 != ec) {
    std::cout << "RelativeMouseButtonRelease(X2) = " << ec << '\n';
  }
#endif

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ec = cgvhid_client.MouseReset();
  if (0 != ec) {
    std::cout << "MouseReset() = " << ec << '\n';
  }
}

int main() {
  CgvhidClient cgvhid_client;

  cgvhid_client.Init(3300, 2200);

  // TestKeyboard(cgvhid_client);
  TestMouse(cgvhid_client);

  return 0;
}
