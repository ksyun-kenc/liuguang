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

#include "captureyuv.h"

bool CaptureYuv::Initialize() noexcept {
  ATLTRACE2(atlTraceUtil, 0, "%s: +\n", __func__);

  if (nullptr == sa_.lpSecurityDescriptor) {
    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
            _T("D:P(A;;GA;;;WD)"), SDDL_REVISION_1, &sa_.lpSecurityDescriptor,
            nullptr)) {
      ATLTRACE2(atlTraceException, 0,
                "%s: ConvertStringSecurityDescriptorToSecurityDescriptor "
                "failed with %u.\n",
                __func__, GetLastError());
      return false;
    }
  }

  if (nullptr == stop_event_) {
    HANDLE ev = CreateEvent(SA(), TRUE, FALSE, nullptr);
    if (nullptr == ev) {
      ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
                GetLastError());
      return false;
    }
    stop_event_.Attach(ev);
  }

  if (nullptr == encoder_started_event_) {
    HANDLE ev = CreateEvent(SA(), TRUE, FALSE, kVideoStartedEventName.data());
    if (nullptr == ev) {
      ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
                GetLastError());
      return false;
    }
    encoder_started_event_.Attach(ev);
  }

  if (!CreateSharedFrameInfo()) {
    ATLTRACE2(atlTraceException, 0, "CreateSharedFrameInfo() failed!\n");
    return false;
  }

  ATLTRACE2(atlTraceUtil, 0, "%s: -\n", __func__);
  return true;
}

bool CaptureYuv::Run() noexcept {
  ATLTRACE2(atlTraceUtil, 0, "%s: +\n", __func__);

  if (!Initialize()) {
    return false;
  }

  if (nullptr != hook_thread_.native_handle() &&
      WAIT_TIMEOUT == WaitForSingleObject(hook_thread_.native_handle(), 0)) {
    ATLTRACE2(atlTraceUtil, 0, "%s: already running.\n", __func__);
    return true;
  }

  hook_thread_ = std::thread(&CaptureYuv::HookThread, this);
  if (!hook_thread_.joinable()) {
    ATLTRACE2(atlTraceException, 0, "Create thread failed with %u\n",
              GetLastError());
    return false;
  }

  ATLTRACE2(atlTraceUtil, 0, "%s: -\n", __func__);
  return true;
}

void CaptureYuv::Free() noexcept {
  ATLTRACE2(atlTraceUtil, 0, "%s: +\n", __func__);

  SetEvent(stop_event_);
  if (hook_thread_.joinable()) {
    hook_thread_.join();
  }

  if (is_d3d9_hooked_) {
    hook_d3d9_.Unhook();
  }
  if (is_dxgi_hooked_) {
    hook_dxgi_.Unhook();
  }

  FreeSharedVideoYuvFrames();
  FreeSharedVideoFrameInfo();

  encoder_started_event_.Close();
  stop_event_.Close();

  if (nullptr != sa_.lpSecurityDescriptor) {
    LocalFree(sa_.lpSecurityDescriptor);
    sa_.lpSecurityDescriptor = nullptr;
  }

  ATLTRACE2(atlTraceUtil, 0, "%s: -\n", __func__);
}

bool CaptureYuv::AttemptToHook() noexcept {
  ATLTRACE2(atlTraceUtil, 0, "%s: + id = %u\n", __func__,
            hook_thread_.get_id());

  bool hooked = false;
  if (!is_d3d9_hooked_) {
    if (hook_d3d9_.Hook()) {
      is_d3d9_hooked_ = true;
      hooked = true;
    }
  }

  if (!is_dxgi_hooked_) {
    if (hook_dxgi_.Hook()) {
      is_dxgi_hooked_ = true;
      hooked = true;
    }
  }

  return hooked;
}

int CaptureYuv::HookThread() noexcept {
  ATLTRACE2(atlTraceUtil, 0, "%s: +\n", __func__);

  CHandle donot_present_event;
  HANDLE ev = CreateEvent(SA(), TRUE, FALSE, kDoNotPresentEventName.data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "%s: CreateEvent() failed with %u\n",
              __func__, GetLastError());
  } else {
    donot_present_event.Attach(ev);
  }

  CHandle encoder_stopped_event;
  ev = CreateEvent(SA(), TRUE, FALSE, kVideoStoppedEventName.data());
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "%s: CreateEvent() failed with %u\n",
              __func__, GetLastError());
  } else {
    encoder_stopped_event.Attach(ev);
  }

  for (bool hooked = false;;) {
    HANDLE started_events[] = {stop_event_, encoder_started_event_};
    DWORD wait = WaitForMultipleObjects(_countof(started_events),
                                        started_events, FALSE, INFINITE);
    if (WAIT_OBJECT_0 == wait) {
      ATLTRACE2(atlTraceUtil, 0, "%s: stopping.\n", __func__);
      return 0;
    } else if (WAIT_OBJECT_0 + 1 == wait) {
      is_encoder_started_ = true;
      ATLTRACE2(atlTraceUtil, 0, "%s: Video encoder is started.\n", __func__);
    } else {
      int error_code = GetLastError();
      ATLTRACE2(atlTraceException, 0,
                "%s: WaitForMultipleObjects() return %u, error %u.\n", __func__,
                wait, error_code);
      return error_code;
    }

    if (nullptr != donot_present_event) {
      wait = WaitForSingleObject(donot_present_event, 0);
      if (WAIT_OBJECT_0 == wait) {
        is_present_enabled_ = false;
      } else {
        is_present_enabled_ = true;
      }

      ATLTRACE2(atlTraceUtil, 0,
                "%s: wait donot_present_event = %d, is_present_enabled = %d.\n",
                __func__, wait, is_present_enabled_);
    }

    HANDLE stop_events[] = {stop_event_, encoder_stopped_event};
    if (!hooked) {
      for (;;) {
        if (AttemptToHook()) {
          hooked = true;
          break;
        }
        wait = WaitForMultipleObjects(_countof(stop_events), stop_events, FALSE,
                                      300);
        if (WAIT_OBJECT_0 == wait) {
          ATLTRACE2(atlTraceUtil, 0, "%s: stopping.\n", __func__);
          return 0;
        } else if (WAIT_OBJECT_0 + 1 == wait) {
          break;
        }
      }
    }

    wait = WaitForMultipleObjects(_countof(stop_events), stop_events, FALSE,
                                  INFINITE);
    is_present_enabled_ = true;
    if (WAIT_OBJECT_0 == wait) {
      ATLTRACE2(atlTraceUtil, 0, "%s: stopping.\n", __func__);
      return 0;
    } else if (WAIT_OBJECT_0 + 1 == wait) {
      is_encoder_started_ = false;
      ATLTRACE2(atlTraceUtil, 0, "%s: Video encoder is stopped.\n", __func__);
    } else {
      int error_code = GetLastError();
      ATLTRACE2(atlTraceException, 0,
                "%s: WaitForMultipleObjects() return %u, error %u.\n", __func__,
                wait, error_code);
      return error_code;
    }
  }

  ATLTRACE2(atlTraceUtil, 0, "%s: -\n", __func__);
  return -1;
}

bool CaptureYuv::CreateSharedVideoYuvFrames(size_t data_size) noexcept {
  bool need_create_file_mapping = false;
  size_t new_size =
      sizeof(SharedVideoYuvFrames) + data_size * kNumberOfSharedFrames;
  if (nullptr == shared_frames_) {
    need_create_file_mapping = true;
  } else if (new_size > shared_frames_.GetMappingSize()) {
    shared_frames_.Unmap();
    need_create_file_mapping = true;
  } else {
    // update data_size
    auto shared_frame =
        static_cast<SharedVideoYuvFrames*>(shared_frames_.GetData());
    if (shared_frame->data_size != static_cast<uint32_t>(data_size)) {
      shared_frame->data_size = static_cast<uint32_t>(data_size);
      ATLTRACE2(atlTraceUtil, 0,
                "Require mapping %zu, but old %zu is enough.\n", new_size,
                shared_frames_.GetMappingSize());
    } else {
      ATLTRACE2(atlTraceUtil, 0, "Require mapping %zu, the same as old.\n",
                new_size);
    }
  }

  if (need_create_file_mapping) {
    HRESULT hr = shared_frames_.MapSharedMem(
        new_size, kSharedVideoYuvFramesFileMappingName.data(), nullptr, SA());
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0, "MapSharedMem() failed.\n");
      return false;
    }

    if (nullptr == shared_frame_ready_event_) {
      HANDLE ev = CreateEvent(SA(), FALSE, FALSE,
                              kSharedVideoFrameReadyEventName.data());
      if (nullptr == ev) {
        ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
                  GetLastError());
        return false;
      }
      shared_frame_ready_event_.Attach(ev);
    }

    ATLTRACE2(atlTraceUtil, 0, "MapSharedMem size = %zu + %zu * 2 = %zu\n",
              sizeof(SharedVideoYuvFrames), data_size, new_size);

    auto shared_frame =
        static_cast<SharedVideoYuvFrames*>(shared_frames_.GetData());
    shared_frame->data_size = static_cast<uint32_t>(data_size);
  }
  return true;
}
