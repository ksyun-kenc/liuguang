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

#include "vigem_client.h"

std::unique_ptr<class ViGEmTargetX360> ViGEmClient::CreateController() {
  return std::unique_ptr<ViGEmTargetX360>{
      new ViGEmTargetX360{shared_from_this()}};
}

ViGEmTargetX360::ViGEmTargetX360(std::shared_ptr<ViGEmClient> client) {
  client_ = client;
  target_ = vigem_target_x360_alloc();
  vigem_target_add(client_->GetHandle(), target_);
  vigem_target_x360_register_notification(client_->GetHandle(), target_,
                                          &ControllerNotification, this);
}

ViGEmTargetX360::~ViGEmTargetX360() {
  std::unique_lock<std::mutex> lock(mutex_);

  vigem_target_x360_unregister_notification(target_);
  vigem_target_remove(client_->GetHandle(), target_);
  vigem_target_free(target_);
}

void ViGEmTargetX360::SetState(const XINPUT_GAMEPAD& gamepad) {
  std::unique_lock<std::mutex> lock(mutex_);

  XUSB_REPORT report;
  report.wButtons = gamepad.wButtons;
  report.bLeftTrigger = gamepad.bLeftTrigger;
  report.bRightTrigger = gamepad.bRightTrigger;
  report.sThumbLX = gamepad.sThumbLX;
  report.sThumbLY = gamepad.sThumbLY;
  report.sThumbRX = gamepad.sThumbRX;
  report.sThumbRY = gamepad.sThumbRY;
  vigem_target_x360_update(client_->GetHandle(), target_, report);
}

bool ViGEmTargetX360::GetVibration(XINPUT_VIBRATION* vibration) {
  assert(nullptr != vibration);

  std::unique_lock<std::mutex> lock(mutex_);
  if (has_pending_vibration_) {
    *vibration = pending_vibration_;
    has_pending_vibration_ = false;
    return true;
  }
  return false;
}

void ViGEmTargetX360::ControllerNotification(PVIGEM_CLIENT client,
                                             PVIGEM_TARGET target,
                                             UCHAR large_motor,
                                             UCHAR small_motor,
                                             UCHAR led_number,
                                             LPVOID context) {
  auto _this = static_cast<ViGEmTargetX360*>(context);

  std::unique_lock<std::mutex> lock(_this->mutex_);
  _this->pending_vibration_.wLeftMotorSpeed = large_motor << 8;
  _this->pending_vibration_.wRightMotorSpeed = small_motor << 8;
  _this->has_pending_vibration_ = true;
}
