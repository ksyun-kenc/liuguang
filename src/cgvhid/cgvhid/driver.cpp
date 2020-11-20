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

/*++                                              \
 Module Name:
    driver.cpp

Abstract:
    This file contains the driver entry points and callbacks.

Environment:
    Kernel-mode Driver Framework
--*/

#include "driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, CgvhidEvtDeviceAdd)
#pragma alloc_text(PAGE, CgvhidEvtDriverContextCleanup)
#endif

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT driver_object,
                     _In_ PUNICODE_STRING registry_path) {
  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));

  WDF_OBJECT_ATTRIBUTES attributes;
  WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
  attributes.EvtCleanupCallback = CgvhidEvtDriverContextCleanup;

  WDF_DRIVER_CONFIG config;
  WDF_DRIVER_CONFIG_INIT(&config, CgvhidEvtDeviceAdd);

  NTSTATUS status = WdfDriverCreate(driver_object, registry_path, &attributes,
                                    &config, WDF_NO_HANDLE);
  if (!NT_SUCCESS(status)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": !WdfDriverCreate() #0x%x", status));
    return status;
  }

  return status;
}

NTSTATUS CgvhidEvtDeviceAdd(_In_ WDFDRIVER driver,
                            _Inout_ PWDFDEVICE_INIT device_init) {
  UNREFERENCED_PARAMETER(driver);

  PAGED_CODE();

  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));

  return CgvhidCreateDevice(device_init);
}

VOID CgvhidEvtDriverContextCleanup(_In_ WDFOBJECT driver_object) {
  UNREFERENCED_PARAMETER(driver_object);
  PAGED_CODE();

  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));
}
