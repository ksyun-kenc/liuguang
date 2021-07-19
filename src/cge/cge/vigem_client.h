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

#include <Xinput.h>

#include "ViGEm/Client.h"

class ViGEmClient : public std::enable_shared_from_this<ViGEmClient> {
 public:
  ViGEmClient() noexcept {
    client_ = vigem_alloc();
    VIGEM_ERROR ec = vigem_connect(client_);
    if (VIGEM_ERROR_NONE != ec) {
      vigem_free(client_);
      client_ = nullptr;
      APP_ERROR() << "vigem_connect() failed with 0x" << std::hex << ec << '\n';
    }
  }

  ~ViGEmClient() {
    if (nullptr != client_) {
      vigem_free(client_);
    }
  }

  std::unique_ptr<class ViGEmTargetX360> CreateController();

  const PVIGEM_CLIENT GetHandle() const noexcept { return client_; }

 private:
  PVIGEM_CLIENT client_ = nullptr;
};

class ViGEmTargetX360 {
  friend class ViGEmClient;

 private:
  ViGEmTargetX360(std::shared_ptr<ViGEmClient> client);

 public:
  ~ViGEmTargetX360();

  void SetState(const XINPUT_GAMEPAD& gamepad);
  bool GetVibration(XINPUT_VIBRATION* vibration);

 private:
  static void CALLBACK ControllerNotification(PVIGEM_CLIENT client,
                                              PVIGEM_TARGET target,
                                              UCHAR large_motor,
                                              UCHAR small_motor,
                                              UCHAR led_number,
                                              LPVOID context);

  std::shared_ptr<ViGEmClient> client_;
  PVIGEM_TARGET target_;
  XINPUT_VIBRATION pending_vibration_{};
  bool has_pending_vibration_ = false;
  std::mutex mutex_;
};
