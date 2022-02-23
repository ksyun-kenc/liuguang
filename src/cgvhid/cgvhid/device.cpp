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

#include "driver.h"

#include "regame/vhid_common.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CgvhidCreateDevice)
#endif

#define VHID_HARDWARE_IDS L"ksyun\\cgvhid\0"
constexpr ULONG kVhidPoolTag = 'DVGC';
constexpr size_t kVhidHardwareIdsLength = sizeof(VHID_HARDWARE_IDS);
constexpr uint8_t kMultiMaxCount = 20;

// clang-format off
// https://eleccelerator.com/tutorial-about-usb-hid-report-descriptors/
#define MT_TOUCH_COLLECTION                                                    \
    0xA1, 0x02,                         /*     COLLECTION (Logical)         */ \
    0x09, 0x42,                         /*       USAGE (Tip Switch)         */ \
    0x15, 0x00,                         /*       LOGICAL_MINIMUM (0)        */ \
    0x25, 0x01,                         /*       LOGICAL_MAXIMUM (1)        */ \
    0x75, 0x01,                         /*       REPORT_SIZE (1)            */ \
    0x95, 0x01,                         /*       REPORT_COUNT (1)           */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x09, 0x32,                         /*       USAGE (In Range)           */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x09, 0x47,                         /*       USAGE (Confidence)         */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x95, 0x05,                         /*       REPORT_COUNT (5)           */ \
    0x81, 0x03,                         /*       INPUT (Cnst,Ary,Abs)       */ \
    0x75, 0x08,                         /*       REPORT_SIZE (8)            */ \
    0x09, 0x51,                         /*       USAGE (Contact Identifier) */ \
    0x95, 0x01,                         /*       REPORT_COUNT (1)           */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x05, 0x01,                         /*       USAGE_PAGE (Generic Desk.. */ \
    0x26, MTOUCH_LOGICAL_MAX_LOW, MTOUCH_LOGICAL_MAX_HIGH,                     \
                                        /*       LOGICAL_MAXIMUM (32767)    */ \
    0x75, 0x10,                         /*       REPORT_SIZE (16)           */ \
    0x55, 0x00,                         /*       UNIT_EXPONENT (0)          */ \
    0x65, 0x00,                         /*       UNIT (None)                */ \
    0x35, 0x00,                         /*       PHYSICAL_MINIMUM (0)       */ \
    0x46, 0x00, 0x00,                   /*       PHYSICAL_MAXIMUM (0)       */ \
    0x09, 0x30,                         /*       USAGE (X)                  */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x09, 0x31,                         /*       USAGE (Y)                  */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x05, 0x0D,                         /*       USAGE PAGE (Digitizers)    */ \
    0x09, 0x48,                         /*       USAGE (Width)              */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x09, 0x49,                         /*       USAGE (Height)             */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0xC0,                               /*    END_COLLECTION                */

//
// This is the default report descriptor for the Hid device provided
// by the mini driver in response to IOCTL_HID_GET_REPORT_DESCRIPTOR.
// 

HID_REPORT_DESCRIPTOR g_vhid_report_descriptor[] = {
//
// Multitouch report starts here
//
    0x05, 0x0D,                         // USAGE_PAGE (Digitizers)
    0x09, 0x04,                         // USAGE (Touch Screen)
    0xA1, 0x01,                         // COLLECTION (Application)
    0x85, REPORT_ID_MTOUCH,             //   REPORT_ID (Touch)
    0x09, 0x22,                         //   USAGE (Finger)
    MT_TOUCH_COLLECTION
    MT_TOUCH_COLLECTION
    0x05, 0x0D,                         //    USAGE_PAGE (Digitizers)
    0x09, 0x54,                         //    USAGE (Contact Count)
    0x95, 0x01,                         //    REPORT_COUNT (1)
    0x75, 0x08,                         //    REPORT_SIZE (8)
    0x15, 0x00,                         //    LOGICAL_MINIMUM (0)
    0x25, 0x08,                         //    LOGICAL_MAXIMUM (8)
    0x81, 0x02,                         //    INPUT (Data,Var,Abs)
    0x09, 0x55,                         //    USAGE(Contact Count Maximum)
    0xB1, 0x02,                         //    FEATURE (Data,Var,Abs)
    0xC0,                               // END_COLLECTION

//
// Feature report starts here
//
    0x09, 0x0E,                         // USAGE (Device Configuration)
    0xA1, 0x01,                         // COLLECTION (Application)
    0x85, REPORT_ID_FEATURE,            //   REPORT_ID (Configuration)              
    0x09, 0x23,                         //   USAGE (Device Settings)              
    0xA1, 0x02,                         //   COLLECTION (logical)    
    0x09, 0x52,                         //    USAGE (Device Mode)         
    0x09, 0x53,                         //    USAGE (Device Identifier)
    0x15, 0x00,                         //    LOGICAL_MINIMUM (0)      
    0x25, 0x0A,                         //    LOGICAL_MAXIMUM (10)
    0x75, 0x08,                         //    REPORT_SIZE (8)         
    0x95, 0x02,                         //    REPORT_COUNT (2)         
    0xB1, 0x02,                         //   FEATURE (Data,Var,Abs)    
    0xC0,                               //   END_COLLECTION
    0xC0,                               // END_COLLECTION

//
// Mouse report starts here
//
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop) 
    0x09, 0x02,                         // USAGE (Mouse)               
    0xA1, 0x01,                         // COLLECTION (Application)   
    0x85, REPORT_ID_MOUSE,              //   REPORT_ID (Mouse)       
    0x09, 0x01,                         //   USAGE (Pointer)        
    0xA1, 0x00,                         //   COLLECTION (Physical) 
    0x05, 0x09,                         //     USAGE_PAGE (Button)
    0x19, 0x01,                         //     USAGE_MINIMUM (Button 1) 
    0x29, 0x05,                         //     USAGE_MAXIMUM (Button 5)
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)    
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)   
    0x75, 0x01,                         //     REPORT_SIZE (1)      
    0x95, 0x05,                         //     REPORT_COUNT (5)    
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0x95, 0x03,                         //     REPORT_COUNT (3)   
    0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)    
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop)
    0x26, MOUSE_LOGICAL_MAX_LOW, MOUSE_LOGICAL_MAX_HIGH,
                                        //     LOGICAL_MAXIMUM (32767)    
    0x75, 0x10,                         //     REPORT_SIZE (16)
    0x95, 0x01,                         //     REPORT_COUNT (1)
    0x55, 0x0F,                         //     UNIT_EXPONENT (-1)
    0x65, 0x11,                         //     UNIT (cm,SI Linear)
    0x35, 0x00,                         //     PHYSICAL_MINIMUM (0)
    0x45, 0x00,                         //     PHYSICAL_MAXIMUM (0)
    0x09, 0x30,                         //     USAGE (X)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0x09, 0x31,                         //     USAGE (Y)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0x05, 0x01,                         //     Usage Page (Generic Desktop)
    0x09, 0x38,                         //     Usage (Wheel)
    0x15, MOUSE_WHEEL_LOGICAL_MIN,      //     Logical Minimum (-127)
    0x25, MOUSE_WHEEL_LOGICAL_MAX,      //     Logical Maximum (127)
    0x75, 0x08,                         //     Report Size (8)
    0x95, 0x01,                         //     Report Count (1)
    0x81, 0x06,                         //     Input (Data, Variable, Relative)
    0xC0,                               //   END_COLLECTION              
    0xC0,                               // END_COLLECTION     

//
// Relative mouse report starts here
//
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                         // USAGE (Mouse)
    0xA1, 0x01,                         // COLLECTION (Application)
    0x85, REPORT_ID_RMOUSE,             //   REPORT_ID (Mouse)
    0x09, 0x01,                         //   USAGE (Pointer)
    0xA1, 0x00,                         //   COLLECTION (Physical)
    0x05, 0x09,                         //     USAGE_PAGE (Button)
    0x19, 0x01,                         //     USAGE_MINIMUM (Button 1)
    0x29, 0x05,                         //     USAGE_MAXIMUM (Button 5)
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //     REPORT_SIZE (1)
    0x95, 0x05,                         //     REPORT_COUNT (5)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0x95, 0x03,                         //     REPORT_COUNT (3)
    0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                         //     USAGE (X)
    0x09, 0x31,                         //     USAGE (Y)
    0x15, RMOUSE_LOGICAL_MIN,           //     Logical Minimum (-127)
    0x25, RMOUSE_LOGICAL_MAX,           //     Logical Maximum (127)
    0x75, 0x08,                         //     REPORT_SIZE (8)
    0x95, 0x02,                         //     REPORT_COUNT (2)
    0x81, 0x06,                         //     INPUT (Data,Var,Rel)
    0x05, 0x01,                         //     Usage Page (Generic Desktop)
    0x09, 0x38,                         //     Usage (Wheel)
    0x15, MOUSE_WHEEL_LOGICAL_MIN,      //     Logical Minimum (-127)
    0x25, MOUSE_WHEEL_LOGICAL_MAX,      //     Logical Maximum (127)
    0x75, 0x08,                         //     Report Size (8)
    0x95, 0x01,                         //     Report Count (1)
    0x81, 0x06,                         //     Input (Data, Variable, Relative)
    0xC0,                               //   END_COLLECTION
    0xC0,                               // END_COLLECTION

//
// Digitizer report starts here
//
    0x05, 0x0D,                         // USAGE_PAGE (Digitizers)
    0x09, 0x02,                         // USAGE (Pen digitizer)
    0xA1, 0x01,                         // COLLECTION (Application)
    0x85, REPORT_ID_DIGI,               //   REPORT_ID (Digi)
    0x05, 0x0D,                         //   USAGE_PAGE (Digitizers)
    0x09, 0x20,                         //   USAGE (Stylus)
    0xA1, 0x00,                         //   COLLECTION (Physical)
    0x09, 0x42,                         //     USAGE (Tip Switch)
    0x09, 0x32,                         //     USAGE (In Range)
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //     REPORT_SIZE (1)
    0x95, 0x02,                         //     REPORT_COUNT (2)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0x75, 0x01,                         //     REPORT_SIZE (1)
    0x95, 0x06,                         //     REPORT_COUNT (6)
    0x81, 0x01,                         //     INPUT (Cnst,Ary,Abs)
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop)
    0x26, DIGI_LOGICAL_MAX_LOW, DIGI_LOGICAL_MAX_HIGH,
                                        //     LOGICAL_MAXIMUM (32767)       
    0x75, 0x10,                         //     REPORT_SIZE (16) 
    0x95, 0x01,                         //     REPORT_COUNT (1)            
    0x55, 0x0F,                         //     UNIT_EXPONENT (-1)           
    0x65, 0x11,                         //     UNIT (cm,SI Linear)                  
    0x35, 0x00,                         //     PHYSICAL_MINIMUM (0)         
    0x45, 0x00,                         //     PHYSICAL_MAXIMUM (0)
    0x09, 0x30,                         //     USAGE (X)                    
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)         
    0x09, 0x31,                         //     USAGE (Y)                    
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0xC0,                               //   END_COLLECTION
    0xC0,                               // END_COLLECTION

//
// Joystick report starts here
//
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x09, 0x04,                         // USAGE (Joystick)
    0xA1, 0x01,                         // COLLECTION (Application)
    0x85, REPORT_ID_JOYSTICK,           //   REPORT_ID (Joystick)
    
    0x05, 0x02,                         //   USAGE_PAGE (Simulation Controls)
    0x09, 0xBB,                         //   USAGE (Throttle)
    0x15, 0x81,                         //   LOGICAL_MINIMUM (-127)
    0x25, 0x7F,                         //   LOGICAL_MAXIMUM (127)
    0x75, 0x08,                         //   REPORT_SIZE (8)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)

    0x05, 0x01,                         //   USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                         //   USAGE (Pointer)
    0xA1, 0x00,                         //   COLLECTION (Physical)
    0x09, 0x30,                         //     USAGE (X)
    0x09, 0x31,                         //     USAGE (Y)
    0x95, 0x02,                         //     REPORT_COUNT (2)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0xC0,                               //   END_COLLECTION

    0xA1, 0x00,                         //   COLLECTION (Physical)
    0x09, 0x39,                         //     USAGE (Hat switch)
    0x15, 0x01,                         //     LOGICAL_MINIMUM (1)
    0x26, 0xFF, 0x00,                   //     LOGICAL_MAXIMUM (255)
    0x35, 0x00,                         //     PHYSICAL_MINIMUM (0)
    0x46, 0x0E, 0x01,                   //     PHYSICAL_MAXIMUM (270)
    0x65, 0x14,                         //     UNIT (Eng Rot:Angular Pos)
    0x75, 0x08,                         //     REPORT_SIZE (8)
    0x95, 0x01,                         //     REPORT_COUNT (1)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)

    0x09, 0x33,                         //     USAGE (Rx)
    0x95, 0x01,                         //     REPORT_COUNT (1)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
 
    0x09, 0x34,                         //     USAGE (Ry)
    0x95, 0x01,                         //     REPORT_COUNT (1)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0xC0,                               //   END_COLLECTION

    0x05, 0x09,                         //   USAGE_PAGE (Button)
    0x19, 0x01,                         //   USAGE_MINIMUM (Button 1)
    0x29, 0x10,                         //   USAGE_MAXIMUM (Button 16)
    0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x95, 0x10,                         //   REPORT_COUNT (16)
    0x55, 0x00,                         //   UNIT_EXPONENT (0)
    0x65, 0x00,                         //   UNIT (None)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0xC0,                               // END_COLLECTION

//
// Gamepad starts here (base XBox)
//
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, REPORT_ID_GAMEPAD_IN,        //   Report ID (1)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x15, 0x00,        //     Logical Minimum (0)
    0x27, GAMEPAD_XY_LOGICAL_MAX_LOW, GAMEPAD_XY_LOGICAL_MAX_HIGH, 0x00, 0x00,  //     Logical Maximum (65535)
    0x95, 0x02,        //     Report Count (2)
    0x75, 0x10,        //     Report Size (16)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x09, 0x33,        //     Usage (Rx)
    0x09, 0x34,        //     Usage (Ry)
    0x15, 0x00,        //     Logical Minimum (0)
    0x27, GAMEPAD_XY_LOGICAL_MAX_LOW, GAMEPAD_XY_LOGICAL_MAX_HIGH, 0x00, 0x00,  //     Logical Maximum (65535)
    0x95, 0x02,        //     Report Count (2)
    0x75, 0x10,        //     Report Size (16)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x32,        //   Usage (Z)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, GAMEPAD_Z_LOGICAL_MAX_LOW, GAMEPAD_Z_LOGICAL_MAX_HIGH,  //   Logical Maximum (1023)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x0A,        //   Report Size (10)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x00,        //   Logical Maximum (0)
    0x75, 0x06,        //   Report Size (6)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x35,        //   Usage (Rz)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, GAMEPAD_Z_LOGICAL_MAX_LOW, GAMEPAD_Z_LOGICAL_MAX_HIGH,  //   Logical Maximum (1023)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x0A,        //   Report Size (10)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x00,        //   Logical Maximum (0)
    0x75, 0x06,        //   Report Size (6)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x39,        //   Usage (Hat switch)
    0x15, GAMEPAD_HAT_LOGICAL_MIN,        //   Logical Minimum (1)
    0x25, GAMEPAD_HAT_LOGICAL_MAX,        //   Logical Maximum (8)
    0x35, 0x00,        //   Physical Minimum (0)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x66, 0x14, 0x00,  //   Unit (System: English Rotation, Length: Centimeter)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x00,        //   Logical Maximum (0)
    0x35, 0x00,        //   Physical Minimum (0)
    0x45, 0x00,        //   Physical Maximum (0)
    0x65, 0x00,        //   Unit (None)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, GAMEPAD_BTN_USAGE_MIN,        //   Usage Minimum (0x01)
    0x29, GAMEPAD_BTN_USAGE_MAX,        //   Usage Maximum (0x0A)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x0A,        //   Report Count (10)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x00,        //   Logical Maximum (0)
    0x75, 0x06,        //   Report Size (6)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x80,        //   Usage (Sys Control)
    0x85, REPORT_ID_GAMEPAD_SYS_MENU,        //   Report ID (2)
    0xA1, 0x00,        //   Collection (Physical)
    0x09, 0x85,        //     Usage (Sys Main Menu)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x00,        //     Logical Maximum (0)
    0x75, 0x07,        //     Report Size (7)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x05, 0x0F,        //   Usage Page (PID Page)
    0x09, 0x21,        //   Usage (0x21)
    0x85, REPORT_ID_GAMEPAD_OUT,        //   Report ID (3)
    0xA1, 0x02,        //   Collection (Logical)
    0x09, 0x97,        //     Usage (0x97)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x04,        //     Report Size (4)
    0x95, 0x01,        //     Report Count (1)
    0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x00,        //     Logical Maximum (0)
    0x75, 0x04,        //     Report Size (4)
    0x95, 0x01,        //     Report Count (1)
    0x91, 0x03,        //     Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x09, 0x70,        //     Usage (0x70)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x64,        //     Logical Maximum (100)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x04,        //     Report Count (4)
    0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x09, 0x50,        //     Usage (0x50)
    0x66, 0x01, 0x10,  //     Unit (System: SI Linear, Time: Seconds)
    0x55, 0x0E,        //     Unit Exponent (-2)
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0xFF, 0x00,  //     Logical Maximum (255)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x09, 0xA7,        //     Usage (0xA7)
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0xFF, 0x00,  //     Logical Maximum (255)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x65, 0x00,        //     Unit (None)
    0x55, 0x00,        //     Unit Exponent (0)
    0x09, 0x7C,        //     Usage (0x7C)
    0x15, 0x00,        //     Logical Minimum (0)
    0x26, 0xFF, 0x00,  //     Logical Maximum (255)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0x91, 0x02,        //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              //   End Collection
    0x85, REPORT_ID_GAMEPAD_BAT_STR,        //   Report ID (4)
    0x05, 0x06,        //   Usage Page (Generic Dev Ctrls)
    0x09, 0x20,        //   Usage (Battery Strength)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection

//
// Keyboard report starts here
//    
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                         // USAGE (Keyboard)
    0xA1, 0x01,                         // COLLECTION (Application)
    0x85, REPORT_ID_KEYBOARD,           //   REPORT_ID (Keyboard)    
    0x05, 0x07,                         //   USAGE_PAGE (Keyboard)
    0x19, 0xE0,                         //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xE7,                         //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x95, 0x08,                         //   REPORT_COUNT (8)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x75, 0x08,                         //   REPORT_SIZE (8)
    0x81, 0x03,                         //   INPUT (Cnst,Var,Abs)
    0x95, 0x05,                         //   REPORT_COUNT (5)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x05, 0x08,                         //   USAGE_PAGE (LEDs)
    0x19, 0x01,                         //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                         //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                         //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x75, 0x03,                         //   REPORT_SIZE (3)
    0x91, 0x03,                         //   OUTPUT (Cnst,Var,Abs)
    0x95, KEYBD_MAX_KEY_COUNT,          //   REPORT_COUNT (6)
    0x75, 0x08,                         //   REPORT_SIZE (8)
    0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                         //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                         //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                         //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                         //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                         //   INPUT (Data,Ary,Abs)
    0xC0,                               // END_COLLECTION

//
// Vendor defined control report starts here
//
    0x06, 0x00, 0xFF,                    // USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x01,                          // USAGE (Vendor Usage 1)
    0xA1, 0x01,                          // COLLECTION (Application)
    0x85, REPORT_ID_CONTROL,             //   REPORT_ID (1)  
    0x15, 0x00,                          //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,                    //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                          //   REPORT_SIZE  (8)   - bits
    0x95, CONTROL_REPORT_COUNT,          //   REPORT_COUNT (32)  - Bytes
    0x09, 0x02,                          //   USAGE (Vendor Usage 1)
    0x81, 0x02,                          //   INPUT (Data,Var,Abs)
    0x95, CONTROL_REPORT_COUNT,          //   REPORT_COUNT (32)  - Bytes
    0x09, 0x02,                          //   USAGE (Vendor Usage 1)
    0x91, 0x02,                          //   OUTPUT (Data,Var,Abs)
    0xC0,                                // END_COLLECTION

//
// Vendor defined message report starts here
//
    0x06, 0x00, 0xFF,                    // USAGE_PAGE (Vendor Defined Page 1)
    0x09, 0x02,                          // USAGE (Vendor Usage 2)
    0xA1, 0x01,                          // COLLECTION (Application)
    0x85, REPORT_ID_MESSAGE,             //   REPORT_ID (1)  
    0x15, 0x00,                          //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,                    //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                          //   REPORT_SIZE  (8)   - bits
    0x95, MESSAGE_REPORT_COUNT,          //   REPORT_COUNT (32)  - Bytes
    0x09, 0x02,                          //   USAGE (Vendor Usage 1)
    0x81, 0x02,                          //   INPUT (Data,Var,Abs)
    0x95, 0x40,                          //   REPORT_COUNT (64)  - Bytes
    0x09, 0x02,                          //   USAGE (Vendor Usage 1)
    0x91, 0x02,                          //   OUTPUT (Data,Var,Abs)
    0xC0,                                // END_COLLECTION
};
// clang-format on

HID_DESCRIPTOR g_hid_descriptor = {
    0x09,    // length of HID descriptor
    0x21,    // descriptor type == HID 0x21
    0x0100,  // hid spec release
    0x00,    // country code == Not Specified
    0x01,    // number of HID class descriptors
    {
        // DescriptorList[0]
        0x22,                             // report descriptor type 0x22
        sizeof(g_vhid_report_descriptor)  // total length of report descriptor
    }};

struct DefaultQueueContext {
  WDFDEVICE device;
};

struct ReportQueueContext {
  WDFDEVICE device;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DefaultQueueContext, GetDefaultQueueContext);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(ReportQueueContext, GetReportQueueContext);

NTSTATUS EvtDeviceWdmIrpPreprocess(WDFDEVICE device, PIRP irp) {
  PAGED_CODE();

  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));

  // Get a pointer to the current location in the Irp
  PIO_STACK_LOCATION irp_stack = IoGetCurrentIrpStackLocation(irp);

  // This check is required to filter out QUERY_IDs forwarded
  // by the HIDCLASS for the parent FDO. These IDs are sent
  // by PNP manager for the parent FDO if you root-enumerate this driver.
  auto previous_stack = reinterpret_cast<PIO_STACK_LOCATION>(
      (UCHAR*)(irp_stack) + sizeof(IO_STACK_LOCATION));

  // Get the device object
  PDEVICE_OBJECT device_object = WdfDeviceWdmGetDeviceObject(device);

  if (previous_stack->DeviceObject == device_object) {
    return irp->IoStatus.Status;
  }

  NTSTATUS status = STATUS_SUCCESS;
  switch (irp_stack->Parameters.QueryId.IdType) {
    case BusQueryDeviceID:
    case BusQueryHardwareIDs: {
      // HIDClass is asking for child deviceid & hardwareids.
      // Let us just make up some id for our child device.
      auto buffer = reinterpret_cast<PWCHAR>(ExAllocatePoolWithTag(
          NonPagedPool, kVhidHardwareIdsLength, kVhidPoolTag));

      if (buffer) {
        // Do the copy, store the buffer in the Irp
        RtlCopyMemory(buffer, VHID_HARDWARE_IDS, kVhidHardwareIdsLength);
        irp->IoStatus.Information = (ULONG_PTR)buffer;
        status = STATUS_SUCCESS;
      } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
      }

      irp->IoStatus.Status = status;
      // We don't need to forward this to our bus. This query
      // is for our child so we should complete it right here.
      // fallthru.
      IoCompleteRequest(irp, IO_NO_INCREMENT);
      break;
    }
    default:
      status = irp->IoStatus.Status;
      IoCompleteRequest(irp, IO_NO_INCREMENT);
      break;
  }

  return status;
}

NTSTATUS RequestBufferCopy(_In_ WDFREQUEST request,
                           _In_ PVOID source,
                           _In_ size_t length) {
  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));

  if (length <= 0) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": length = %u", length));
    return STATUS_INVALID_BUFFER_SIZE;
  }

  //
  // This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
  // will correctly retrieve buffer from Irp->UserBuffer.
  // Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
  // field irrespective of the ioctl buffer type. However, framework is very
  // strict about type checking. You cannot get Irp->UserBuffer by using
  // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
  // internal ioctl.
  //
  WDFMEMORY memory = nullptr;
  NTSTATUS status = WdfRequestRetrieveOutputMemory(request, &memory);
  if (!NT_SUCCESS(status)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": !WdfRequestRetrieveOutputMemory() #0x%x",
               status));
    return status;
  }

  size_t out_length = 0;
  WdfMemoryGetBuffer(memory, &out_length);
  if (out_length < length) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": out_length(%u) < length(%u)", out_length,
               length));
    return STATUS_INVALID_BUFFER_SIZE;
  }

  status = WdfMemoryCopyFromBuffer(memory, 0, source, length);
  if (!NT_SUCCESS(status)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": !WdfMemoryCopyFromBuffer() #0x%x", status));
    return status;
  }
  //
  // Report how many bytes were copied
  //
  WdfRequestSetInformation(request, length);
  return status;
}

NTSTATUS IoctlHidGetString(IN WDFREQUEST request) {
  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));

  WDF_REQUEST_PARAMETERS params;
  WDF_REQUEST_PARAMETERS_INIT(&params);
  WdfRequestGetParameters(request, &params);

#define SET_STRING(x) \
  str = x;            \
  length = sizeof(x)

  PWSTR str = nullptr;
  size_t length = 0;
  switch (reinterpret_cast<ULONG_PTR>(
              params.Parameters.DeviceIoControl.Type3InputBuffer) &
          0xFFFF) {
    case HID_STRING_ID_IMANUFACTURER:
      SET_STRING(L"Ksyun");
      break;

    case HID_STRING_ID_IPRODUCT:
      SET_STRING(L"Cloud Gaming Virtual HID Device");
      break;

    case HID_STRING_ID_ISERIALNUMBER:
      SET_STRING(L"KENC-38362-39849");
      break;

    default:
      break;
  }

  return RequestBufferCopy(request, str, length);
}

NTSTATUS IoctlHidSetFeature(IN PDEVICE_CONTEXT device_context,
                            IN WDFREQUEST request) {
  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));

  WDF_REQUEST_PARAMETERS params;
  WDF_REQUEST_PARAMETERS_INIT(&params);
  WdfRequestGetParameters(request, &params);
  if (params.Parameters.DeviceIoControl.InputBufferLength <
      sizeof(HID_XFER_PACKET)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": Xfer packet too small"));
    return STATUS_BUFFER_TOO_SMALL;
  }

  auto packet = reinterpret_cast<PHID_XFER_PACKET>(
      WdfRequestWdmGetIrp(request)->UserBuffer);
  if (nullptr == packet) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": No xfer packet"));
    return STATUS_INVALID_DEVICE_REQUEST;
  }

  if (REPORT_ID_FEATURE != packet->reportId) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": Unhandled reportId %d", packet->reportId));
    return STATUS_INVALID_PARAMETER;
  }

  if (sizeof(VhidFeatureReport) != packet->reportBufferLen) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": Invild reportBufferLen %u, should be %u",
               packet->reportBufferLen, sizeof(VhidFeatureReport)));
    return STATUS_INVALID_PARAMETER;
  }

  auto report = reinterpret_cast<VhidFeatureReport*>(packet->reportBuffer);
  device_context->device_mode = report->dev_mode;
  return STATUS_SUCCESS;
}

NTSTATUS IoctlHidGetFeature(IN PDEVICE_CONTEXT device_context,
                            IN WDFREQUEST request) {
  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));

  WDF_REQUEST_PARAMETERS params;
  WDF_REQUEST_PARAMETERS_INIT(&params);
  WdfRequestGetParameters(request, &params);
  if (params.Parameters.DeviceIoControl.OutputBufferLength <
      sizeof(HID_XFER_PACKET)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": Xfer packet too small"));
    return STATUS_BUFFER_TOO_SMALL;
  }

  auto packet = reinterpret_cast<PHID_XFER_PACKET>(
      WdfRequestWdmGetIrp(request)->UserBuffer);
  if (nullptr == packet) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": No xfer packet"));
    return STATUS_INVALID_DEVICE_REQUEST;
  }

  if (REPORT_ID_MTOUCH == packet->reportId) {
    if (sizeof(VhidMaxCountReport) != packet->reportBufferLen) {
      KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                 DRIVER_NAME ": Invild reportBufferLen %u, should be %u",
                 packet->reportBufferLen, sizeof(VhidMaxCountReport)));
      return STATUS_INVALID_PARAMETER;
    }

    auto report = reinterpret_cast<VhidMaxCountReport*>(packet->reportBuffer);
    report->max_count = kMultiMaxCount;
  } else if (REPORT_ID_FEATURE == packet->reportId) {
    if (sizeof(VhidFeatureReport) != packet->reportBufferLen) {
      KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                 DRIVER_NAME ": Invild reportBufferLen %u, should be %u",
                 packet->reportBufferLen, sizeof(VhidFeatureReport)));
      return STATUS_INVALID_PARAMETER;
    }

    auto report = reinterpret_cast<VhidFeatureReport*>(packet->reportBuffer);
    report->dev_mode = device_context->device_mode;
    report->dev_id = 0;
  } else {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": Unhandled reportId %d", packet->reportId));
    return STATUS_INVALID_PARAMETER;
  }

  return STATUS_SUCCESS;
}

NTSTATUS SendSubReport(IN PDEVICE_CONTEXT device_context,
                       IN PVOID sub_report,
                       IN ULONG sub_report_length,
                       OUT size_t* written) {
  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));

  WDFREQUEST request;
  NTSTATUS status =
      WdfIoQueueRetrieveNextRequest(device_context->report_queue, &request);
  if (!NT_SUCCESS(status)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": !WdfIoQueueRetrieveNextRequest() #0x%x", status));
    return status;
  }

  PVOID report_buffer = nullptr;
  size_t length = 0;
  status = WdfRequestRetrieveOutputBuffer(request, sub_report_length,
                                          &report_buffer, &length);
  if (!NT_SUCCESS(status)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": !WdfRequestRetrieveOutputBuffer() #0x%x",
               status));
    return status;
  }

  RtlCopyMemory(report_buffer, sub_report, sub_report_length);
  // Complete read with the number of bytes returned as info
  WdfRequestCompleteWithInformation(request, status, sub_report_length);
  // Return the number of bytes written for the write request completion
  *written = sub_report_length;
  return status;
}

NTSTATUS IoctlWriteReport(IN PDEVICE_CONTEXT device_context,
                          IN WDFREQUEST request) {
  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));

  WDF_REQUEST_PARAMETERS params;
  WDF_REQUEST_PARAMETERS_INIT(&params);
  WdfRequestGetParameters(request, &params);
  if (params.Parameters.DeviceIoControl.InputBufferLength <
      sizeof(HID_XFER_PACKET)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": Xfer packet too small"));
    return STATUS_BUFFER_TOO_SMALL;
  }

  auto packet = reinterpret_cast<PHID_XFER_PACKET>(
      WdfRequestWdmGetIrp(request)->UserBuffer);
  if (nullptr == packet) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": No xfer packet"));
    return STATUS_INVALID_DEVICE_REQUEST;
  }

  if (REPORT_ID_CONTROL != packet->reportId) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": Unhandled reportId %d", packet->reportId));
    return STATUS_INVALID_PARAMETER;
  }

  auto report = reinterpret_cast<VhidControlReport*>(packet->reportBuffer);
  if (report->len > packet->reportBufferLen - sizeof(VhidControlReport)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": Invild report->len %u, should be <= %u",
               report->len,
               packet->reportBufferLen - sizeof(VhidControlReport)));
    return STATUS_INVALID_PARAMETER;
  }

  size_t written = 0;
  NTSTATUS status = SendSubReport(
      device_context, packet->reportBuffer + sizeof(VhidControlReport),
      report->len, &written);
  if (NT_SUCCESS(status)) {
    // Report how many bytes were written
    WdfRequestSetInformation(request, written);
  }
  VhidMessageReport message_report;
  message_report.id = REPORT_ID_MESSAGE;
  message_report.msg[0] = 'C';
  message_report.msg[1] = 'G';
  message_report.msg[2] = 'V';
  message_report.msg[3] = 'D';
  message_report.msg[4] = '\n';
  status = SendSubReport(device_context, &message_report,
                         sizeof(VhidMessageReport), &written);
  return status;
}

_Use_decl_annotations_ VOID InternalIoctl(_In_ WDFQUEUE queue,
                                          _In_ WDFREQUEST reqeust,
                                          _In_ size_t out_length,
                                          _In_ size_t in_length,
                                          _In_ ULONG ioctl_code) {
  UNREFERENCED_PARAMETER(out_length);
  UNREFERENCED_PARAMETER(in_length);

  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));

  NTSTATUS status = STATUS_SUCCESS;
  bool complete_request = true;
  PDEVICE_CONTEXT device_context = GetDeviceContext(WdfIoQueueGetDevice(queue));

  switch (ioctl_code) {
    case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
      status = RequestBufferCopy(reqeust, device_context->hid_descriptor,
                                 device_context->hid_descriptor->bLength);
      break;
    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
      status = RequestBufferCopy(
          reqeust, device_context->hid_report_descriptor,
          device_context->hid_descriptor->DescriptorList[0].wReportLength);
      break;
    case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
      status =
          RequestBufferCopy(reqeust, &device_context->hid_device_attributes,
                            sizeof(device_context->hid_device_attributes));
      break;
    case IOCTL_HID_GET_STRING:
      //
      // Requests that the HID minidriver retrieve a human-readable string
      // for either the manufacturer ID, the product ID, or the serial
      // number from the string descriptor of the device. The minidriver
      // must send a Get String Descriptor request to the device, in order
      // to retrieve the string descriptor, then it must extract the
      // string at the appropriate index from the string descriptor and
      // return it in the output buffer indicated by the IRP. Before
      // sending the Get String Descriptor request, the minidriver must
      // retrieve the appropriate index for the manufacturer ID, the
      // product ID or the serial number from the device extension of a
      // top level collection associated with the device.
      //
      status = IoctlHidGetString(reqeust);
      break;
    case IOCTL_HID_SET_FEATURE:
      status = IoctlHidSetFeature(device_context, reqeust);
      break;
    case IOCTL_HID_GET_FEATURE:
      status = IoctlHidGetFeature(device_context, reqeust);
      break;
    case IOCTL_HID_READ_REPORT:
    case IOCTL_HID_GET_INPUT_REPORT:
      //
      // Forward this read request to our manual queue
      // (in other words, we are going to defer this request
      // until we have a corresponding write request to
      // match it with)
      //
      status =
          WdfRequestForwardToIoQueue(reqeust, device_context->report_queue);
      if (NT_SUCCESS(status)) {
        complete_request = false;
      }
      break;
    case IOCTL_HID_WRITE_REPORT:
    case IOCTL_HID_SET_OUTPUT_REPORT:
      status = IoctlWriteReport(device_context, reqeust);
      break;
    default:
      status = STATUS_NOT_IMPLEMENTED;
      break;
  }

  if (complete_request) {
    WdfRequestComplete(reqeust, status);
  }
  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s - %s",
             __func__, complete_request ? "completed" : "deferred"));
}

NTSTATUS CreateDefaultQueue(_In_ WDFDEVICE device, _Out_ WDFQUEUE* queue) {
  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));

  WDF_IO_QUEUE_CONFIG config;
  WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&config, WdfIoQueueDispatchSequential);
  config.EvtIoInternalDeviceControl = InternalIoctl;

  WDF_OBJECT_ATTRIBUTES attributes;
  WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DefaultQueueContext);

  WDFQUEUE q = nullptr;
  NTSTATUS status = WdfIoQueueCreate(device, &config, &attributes, &q);
  if (!NT_SUCCESS(status)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": !WdfIoQueueCreate() #0x%x", status));
    return status;
  }

  DefaultQueueContext* context = GetDefaultQueueContext(q);
  context->device = device;
  *queue = q;

  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s -",
             __func__));
  return status;
}

_Use_decl_annotations_ NTSTATUS CreateReportQueue(_In_ WDFDEVICE device,
                                                  _Out_ WDFQUEUE* queue) {
  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));

  WDF_IO_QUEUE_CONFIG config;
  WDF_IO_QUEUE_CONFIG_INIT(&config, WdfIoQueueDispatchManual);
  config.PowerManaged = WdfFalse;

  WDF_OBJECT_ATTRIBUTES attributes;
  WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, ReportQueueContext);

  WDFQUEUE q = nullptr;
  NTSTATUS status = WdfIoQueueCreate(device, &config, &attributes, &q);
  if (!NT_SUCCESS(status)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": !WdfIoQueueCreate() #0x%x", status));
    return status;
  }

  ReportQueueContext* context = GetReportQueueContext(q);
  context->device = device;
  *queue = q;

  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s -",
             __func__));
  return status;
}

NTSTATUS CgvhidCreateDevice(_Inout_ PWDFDEVICE_INIT device_init) {
  PAGED_CODE();

  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s +",
             __func__));

  WdfFdoInitSetFilter(device_init);

  UCHAR minor_functions[] = {IRP_MN_QUERY_ID};
  static_assert(ARRAYSIZE(minor_functions) == 1);
  NTSTATUS status = WdfDeviceInitAssignWdmIrpPreprocessCallback(
      device_init, EvtDeviceWdmIrpPreprocess, IRP_MJ_PNP, minor_functions,
      ARRAYSIZE(minor_functions));
  if (!NT_SUCCESS(status)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME
               ": !WdfDeviceInitAssignWdmIrpPreprocessCallback() #0x%x",
               status));
    return status;
  }

  WDF_OBJECT_ATTRIBUTES device_attributes;
  WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&device_attributes, DEVICE_CONTEXT);
  WDFDEVICE device = nullptr;
  status = WdfDeviceCreate(&device_init, &device_attributes, &device);
  if (!NT_SUCCESS(status)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": !WdfDeviceCreate() #0x%x", status));
    return status;
  }

  PDEVICE_CONTEXT device_context = GetDeviceContext(device);
  device_context->device = device;
  device_context->default_queue = nullptr;
  device_context->report_queue = nullptr;
  device_context->hid_descriptor = &g_hid_descriptor;
  device_context->hid_report_descriptor = g_vhid_report_descriptor;
  RtlZeroMemory(&device_context->hid_device_attributes,
                sizeof(HID_DEVICE_ATTRIBUTES));
  device_context->hid_device_attributes.Size = sizeof(HID_DEVICE_ATTRIBUTES);
  device_context->hid_device_attributes.VendorID = VHID_VENDOR_ID;
  device_context->hid_device_attributes.ProductID = VHID_PRODUCT_ID;
  device_context->hid_device_attributes.VersionNumber = VHID_VERSION_NUMBER;
  device_context->device_mode = DEVICE_MODE_MOUSE;

  status = CreateDefaultQueue(device, &device_context->default_queue);
  if (!NT_SUCCESS(status)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": !CreateDefaultQueue() #0x%x", status));
    return status;
  }

  status = CreateReportQueue(device, &device_context->report_queue);
  if (!NT_SUCCESS(status)) {
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
               DRIVER_NAME ": !CreateDefaultQueue() #0x%x", status));
    return status;
  }

  KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_TRACE_LEVEL, DRIVER_NAME ": %s -",
             __func__));
  return status;
}
