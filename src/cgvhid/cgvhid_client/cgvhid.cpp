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

#include "cgvhid.h"

#include <atlfile.h>

#include <boost/scope_exit.hpp>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "SetupAPI.lib")

HANDLE Cgvhid::Open(USAGE usage_page, USAGE usage) noexcept {
  GUID hid_guid;
  HidD_GetHidGuid(&hid_guid);
  HDEVINFO dev_info = SetupDiGetClassDevs(
      &hid_guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
  if (INVALID_HANDLE_VALUE == dev_info) {
    ATLTRACE2(atlTraceException, 0, "!SetupDiGetClassDevs() #%u\n",
              GetLastError());
    return dev_info;
  }
  BOOST_SCOPE_EXIT_ALL(&) { SetupDiDestroyDeviceInfoList(dev_info); };

  SP_DEVICE_INTERFACE_DATA device_interface_data{};
  device_interface_data.cbSize = sizeof(device_interface_data);
  for (DWORD i = 0; SetupDiEnumDeviceInterfaces(dev_info, nullptr, &hid_guid, i,
                                                &device_interface_data);
       ++i) {
    HANDLE h =
        OpenInterface(dev_info, &device_interface_data, usage_page, usage);
    if (INVALID_HANDLE_VALUE != h) {
      return h;
    }
  }

  ATLTRACE2(atlTraceException, 0, "Can't find our HID device!\n");
  return INVALID_HANDLE_VALUE;
}

HANDLE Cgvhid::OpenInterface(HDEVINFO dev_info,
                             PSP_DEVICE_INTERFACE_DATA device_interface_data,
                             USAGE usage_page,
                             USAGE usage) noexcept {
  DWORD allocate_length = 0;
  SetupDiGetDeviceInterfaceDetail(dev_info, device_interface_data, nullptr, 0,
                                  &allocate_length, nullptr);
  DWORD error_code = GetLastError();
  if (ERROR_INSUFFICIENT_BUFFER != error_code) {
    ATLTRACE2(atlTraceException, 0, "!SetupDiGetDeviceInterfaceDetail() #%u\n",
              error_code);
    return INVALID_HANDLE_VALUE;
  }

  CHeapPtr<SP_DEVICE_INTERFACE_DETAIL_DATA> data;
  if (!data.AllocateBytes(allocate_length)) {
    error_code = GetLastError();
    ATLTRACE2(atlTraceException, 0, "!AllocateBytes() #%u\n", error_code);
    return INVALID_HANDLE_VALUE;
  }

  data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
  DWORD length = 0;
  if (!SetupDiGetDeviceInterfaceDetail(dev_info, device_interface_data, data,
                                       allocate_length, &length, nullptr)) {
    error_code = GetLastError();
    ATLTRACE2(atlTraceException, 0, "!SetupDiGetDeviceInterfaceDetail() #%u\n",
              error_code);
    return INVALID_HANDLE_VALUE;
  }

  CAtlFile file;
  HRESULT hr = file.Create(data->DevicePath, GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "!Create(%ws) #0x%08x\n", data->DevicePath,
              hr);
    return INVALID_HANDLE_VALUE;
  }

  HIDD_ATTRIBUTES attributes;
  if (!HidD_GetAttributes(file, &attributes)) {
    ATLTRACE2(atlTraceException, 0, "!HidD_GetAttributes()\n");
    return INVALID_HANDLE_VALUE;
  }

  if (attributes.VendorID == VHID_VENDOR_ID &&
      attributes.ProductID == VHID_PRODUCT_ID) {
    PHIDP_PREPARSED_DATA pd = nullptr;
    if (!HidD_GetPreparsedData(file, &pd)) {
      ATLTRACE2(atlTraceException, 0, "!HidD_GetPreparsedData()\n");
      return INVALID_HANDLE_VALUE;
    }
    BOOST_SCOPE_EXIT_ALL(&) { HidD_FreePreparsedData(pd); };

    HIDP_CAPS caps{};
    if (!HidP_GetCaps(pd, &caps)) {
      ATLTRACE2(atlTraceException, 0, "!HidP_GetCaps()\n");
      return INVALID_HANDLE_VALUE;
    }
    if ((caps.UsagePage == usage_page) && (caps.Usage == usage)) {
      ATLTRACE2(atlTraceUtil, 0, "OK: %ws\n", data->DevicePath);
      return file.Detach();
    }
    ATLTRACE2(atlTraceUtil, 0, "Ours, but not the one: %ws\n",
              data->DevicePath);
  }

  return INVALID_HANDLE_VALUE;
}

bool Cgvhid::Init() noexcept {
  HANDLE h = Open(0xff00, 0x0001);
  if (INVALID_HANDLE_VALUE == h) {
    return false;
  }
  control_.Attach(h);
  h = Open(0xff00, 0x0002);
  if (INVALID_HANDLE_VALUE == h) {
    return false;
  }
  message_.Attach(h);

  //
  // Set the buffer count to 10 on the message HID
  //
  if (!HidD_SetNumInputBuffers(message_, 10)) {
    Free();
    ATLTRACE2(atlTraceException, 0, "!HidD_SetNumInputBuffers()\n");
    return false;
  }
  return true;
}

void Cgvhid::Free() noexcept {
  control_.Close();
  message_.Close();
}

int Cgvhid::KeyboardUpdate(uint8_t modifiers,
                           uint8_t key_codes[KEYBD_MAX_KEY_COUNT]) noexcept {
  static_assert(CONTROL_REPORT_SIZE >
                    sizeof(VhidControlReport) + sizeof(VhidKeyboardReport),
                "CONTROL_REPORT_SIZE is too small");
  static_assert(sizeof(key_codes) >= KEYBD_MAX_KEY_COUNT,
                "invalid key_codes size");

  auto control_report = GetControlReport();
  control_report->id = REPORT_ID_CONTROL;
  control_report->len = sizeof(VhidKeyboardReport);

  auto keyboard_report = GetKeyboardReport();
  keyboard_report->id = REPORT_ID_KEYBOARD;
  keyboard_report->modifiers = modifiers;
  static_assert(sizeof(keyboard_report->key_codes) == KEYBD_MAX_KEY_COUNT,
                "key_codes size not equal");
  memcpy(keyboard_report->key_codes, key_codes,
         sizeof(keyboard_report->key_codes));

  return WriteControlReport();
}

int Cgvhid::WriteControlReport() noexcept {
  ULONG written = 0;
  if (!WriteFile(control_, control_report_, CONTROL_REPORT_SIZE, &written,
                 nullptr)) {
    int error_code = GetLastError();
    ATLTRACE2(atlTraceException, 0, "!WriteFile(), #%d\n", error_code);
    return error_code;
  }
  ATLTRACE2(atlTraceUtil, 0, "WriteFile() written %d\n", written);
  return 0;
}
