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

#include <hidport.h>

#define DEVICE_MODE_MOUSE         0x00
#define DEVICE_MODE_SINGLE_INPUT  0x01
#define DEVICE_MODE_MULTI_INPUT   0x02

EXTERN_C_START

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

typedef struct _DEVICE_CONTEXT {
  WDFDEVICE device;
  WDFQUEUE default_queue;
  WDFQUEUE report_queue;

  HID_DEVICE_ATTRIBUTES hid_device_attributes;
  PHID_DESCRIPTOR hid_descriptor;
  PHID_REPORT_DESCRIPTOR hid_report_descriptor;

  BYTE device_mode;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext)

//
// Function to initialize the device and its callbacks
//
NTSTATUS CgvhidCreateDevice(_Inout_ PWDFDEVICE_INIT DeviceInit);

EXTERN_C_END
