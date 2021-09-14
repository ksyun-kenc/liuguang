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

#ifdef _NTDDK_
typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
#else
#include <stdint.h>
#endif

#define VHID_VERSION_NUMBER 0x02
#define VHID_VENDOR_ID 0x4B53
#define VHID_PRODUCT_ID 0x4347

#define REPORT_ID_MTOUCH 0x01
#define REPORT_ID_FEATURE 0x02
#define REPORT_ID_MOUSE 0x03
#define REPORT_ID_RMOUSE 0x04
#define REPORT_ID_DIGI 0x05
#define REPORT_ID_JOYSTICK 0x06
#define REPORT_ID_KEYBOARD 0x07
#define REPORT_ID_GAMEPAD_IN 0x0A
#define REPORT_ID_GAMEPAD_SYS_MENU 0x0B
#define REPORT_ID_GAMEPAD_BAT_STR 0x0C
#define REPORT_ID_GAMEPAD_OUT 0x0D
#define REPORT_ID_MESSAGE 0x20
#define REPORT_ID_CONTROL 0x40

#define MOUSE_LOGICAL_MAX_HIGH 0x7F
#define MOUSE_LOGICAL_MAX_LOW 0xFF
#define MOUSE_LOGICAL_MAX (MOUSE_LOGICAL_MAX_HIGH << 8 | MOUSE_LOGICAL_MAX_LOW)
#define MOUSE_LOGICAL_MIN 0

#define MOUSE_WHEEL_LOGICAL_MAX 0x7F
#define MOUSE_WHEEL_LOGICAL_MIN 0x81

#define RMOUSE_LOGICAL_MAX 0x7F
#define RMOUSE_LOGICAL_MIN 0x81

#define MTOUCH_LOGICAL_MAX_HIGH 0x7F
#define MTOUCH_LOGICAL_MAX_LOW 0xFF
#define MTOUCH_LOGICAL_MAX \
  (MTOUCH_LOGICAL_MAX_HIGH << 8 | MTOUCH_LOGICAL_MAX_LOW)
#define MTOUCH_LOGICAL_MIN 0

#define DIGI_LOGICAL_MAX_HIGH 0x7F
#define DIGI_LOGICAL_MAX_LOW 0xFF
#define DIGI_LOGICAL_MAX (DIGI_LOGICAL_MAX_HIGH << 8 | DIGI_LOGICAL_MAX_LOW)
#define DIGI_LOGICAL_MIN 0

#define GAMEPAD_XY_LOGICAL_MAX_HIGH 0xFF
#define GAMEPAD_XY_LOGICAL_MAX_LOW 0xFF
#define GAMEPAD_XY_LOGICAL_MAX \
  (GAMEPAD_XY_LOGICAL_MAX_HIGH << 8 | GAMEPAD_XY_LOGICAL_MAX_LOW)
#define GAMEPAD_XY_LOGICAL_MIN 0

#define GAMEPAD_Z_LOGICAL_MAX_HIGH 0x03
#define GAMEPAD_Z_LOGICAL_MAX_LOW 0xFF
#define GAMEPAD_Z_LOGICAL_MAX \
  (GAMEPAD_Z_LOGICAL_MAX_HIGH << 8 | GAMEPAD_Z_LOGICAL_MAX_LOW)
#define GAMEPAD_Z_LOGICAL_MIN 0
#define GAMEPAD_HAT_LOGICAL_MIN 0x01
#define GAMEPAD_HAT_LOGICAL_MAX 0x08
#define GAMEPAD_BTN_USAGE_MIN 0x01
#define GAMEPAD_BTN_USAGE_MAX 0x0A

#define KEYBD_MAX_KEY_COUNT 0x06
#define CONTROL_REPORT_COUNT 0x20
#define CONTROL_REPORT_SIZE (CONTROL_REPORT_COUNT + 1)

#define MESSAGE_REPORT_COUNT 0x10

#include <pshpack1.h>
typedef struct VHID_GPAD_IN_REPORT {
  uint8_t id;   // Report ID = REPORT_ID_GAMEPAD_IN
                // Collection: CA:GamePad CP:Pointer
  uint16_t x;   // Usage 0x00010030: X, Value = 0 to 65535
  uint16_t y;   // Usage 0x00010031: Y, Value = 0 to 65535
  uint16_t rx;  // Usage 0x00010033: Rx, Value = 0 to 65535
  uint16_t ry;  // Usage 0x00010034: Ry, Value = 0 to 65535
                // Collection: CA:GamePad

  uint16_t z;   // Usage 0x00010032: Z, Value = 0 to 1023
  uint16_t rz;  // Usage 0x00010035: Rz, Value = 0 to 1023
  uint8_t hat;  // Usage 0x00010039: Hat switch, Value = 1 to 8, Physical =
                // (Value - 1) x 45 in degrees
  uint16_t buttons;
  /*
  uint16_t btn1 : 1;  // Usage 0x00090001: Button 1 Primary/trigger,
                      // Value = 0 to 1
  uint16_t btn2 : 1;  // Usage 0x00090002: Button 2 Secondary, Value =
                      // 0 to 1
  uint16_t btn3 : 1;  // Usage 0x00090003: Button 3 Tertiary, Value = 0 to 1
  uint16_t btn4 : 1;  // Usage 0x00090004: Button 4, Value = 0 to 1
  uint16_t btn5 : 1;  // Usage 0x00090005: Button 5, Value = 0 to 1
  uint16_t btn6 : 1;  // Usage 0x00090006: Button 6, Value = 0 to 1
  uint16_t btn7 : 1;  // Usage 0x00090007: Button 7, Value = 0 to 1
  uint16_t btn8 : 1;  // Usage 0x00090008: Button 8, Value = 0 to 1
  uint16_t btn9 : 1;  // Usage 0x00090009: Button 9, Value = 0 to 1
  uint16_t btn10 : 1; // Usage 0x0009000A: Button 10, Value = 0 to 1
  uint16_t : 6;       // Pad
  */
} VhidGamepadInReport;

typedef struct VHID_GPAD_SYS_MENU_REPORT {
  uint8_t id;        // Report ID = REPORT_ID_GAMEPAD_SYS_MENU
                     // Collection: CA:GamePad CP:SystemControl
  uint8_t sys_menu;  // Usage 0x00010085: System Main Menu, Value = 0 to 1
} VhidGamepadSysMenuReport;

typedef struct VHID_GPAD_BAT_STR_REPORT {
  uint8_t id;  // Report ID = REPORT_ID_GAMEPAD_BAT_STR Collection: CA:GamePad
  uint8_t bat_str;  // Usage 0x00060020: Battery Strength,
                    // Value = 0 to 255
} VhidGamepadBatteryStrengthReport;

typedef struct VHID_GPAD_OUT_REPORT {
  uint8_t id;      // Report ID = REPORT_ID_GAMEPAD_OUT Collection: CA:GamePad
                   // CL:SetEffectReport
  uint8_t enable;  // Usage 0x000F0097: DC Enable Actuators, Value = 0 to 1
  uint8_t magnitude[4];  // Usage 0x000F0070: Magnitude, Value = 0 to 100
  uint8_t duration;  // Usage 0x000F0050: Duration, Value = 0 to 255, Physical
                     // = Value in 10^-2 s units
  uint8_t start_delay;  // Usage 0x000F00A7: Start Delay, Value = 0 to 255,
                        // Physical = Value in 10^-2 s units
  uint8_t loop_count;   // Usage 0x000F007C: Loop Count, Value = 0 to 255
} VhidGamepadOutReport;

typedef struct VHID_CTRL_REPORT {
  uint8_t id;
  uint8_t len;
} VhidControlReport;

typedef struct VHID_ABS_MOUSE_REPORT {
  uint8_t id;
  uint8_t btn;
  uint16_t x;
  uint16_t y;
  uint8_t wheel;
  uint8_t hwheel;
} VhidAbsoluteMouseReport;

typedef struct VHID_RELATIVE_MOUSE_REPORT {
  uint8_t id;
  uint8_t btn;
  uint8_t x;
  uint8_t y;
  uint8_t wheel;
  uint8_t hwheel;
} VhidRelativeMouseReport;

typedef struct VHID_DIGI_REPORT {
  uint8_t id;
  uint8_t status;
  uint16_t x;
  uint16_t y;
} VhidDigitizerReport;

typedef struct TOUCH {
  uint8_t status;
  uint8_t contact_id;
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
} Touch;

typedef struct VHID_MTOUCH_REPORT {
  uint8_t id;
  Touch touch[2];
  uint8_t actual_count;
} VhidMtouchReport;

typedef struct VHID_JOYSTICK_REPORT {
  uint8_t id;
  uint8_t throttle;
  uint8_t x;
  uint8_t y;
  uint8_t hat;
  uint8_t rx;
  uint8_t ry;
  uint16_t buttons;
} VhidJoystickReport;

typedef struct VHID_KEYBD_REPORT {
  uint8_t id;
  uint8_t modifiers;
  uint8_t reserved;
  // See https://www.usb.org/sites/default/files/hut1_21_0.pdf
  // for a list of key codes
  uint8_t key_codes[KEYBD_MAX_KEY_COUNT];
} VhidKeyboardReport;

typedef struct VHID_MSG_REPORT {
  uint8_t id;
  uint8_t msg[MESSAGE_REPORT_COUNT];
} VhidMessageReport;

typedef struct VHID_FEATURE_REPORT {
  uint8_t id;
  uint8_t dev_mode;
  uint8_t dev_id;
} VhidFeatureReport;

typedef struct VHID_MAXCOUNT_REPORT {
  uint8_t id;
  uint8_t max_count;
} VhidMaxCountReport;

typedef union {
  VhidAbsoluteMouseReport absolute_mouse;
  VhidRelativeMouseReport relative_mouse;
  VhidDigitizerReport digitizer;
  VhidMtouchReport mtouch;
  VhidJoystickReport joystick;
  VhidKeyboardReport keyboard;
  VhidMessageReport message;
} SubReport;
#include <poppack.h>
