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

enum class WindowsScancode : std::uint16_t {
  kNone = 0,
  kEscape = 0x01,
  k1 = 0x02,
  k2 = 0x03,
  k3 = 0x04,
  k4 = 0x05,
  k5 = 0x06,
  k6 = 0x07,
  k7 = 0x08,
  k8 = 0x09,
  k9 = 0x0A,
  k0 = 0x0B,
  kMinus = 0x0C,
  kEquals = 0x0D,
  kBackspace = 0x0E,
  kTab = 0x0F,
  kQ = 0x10,
  kW = 0x11,
  kE = 0x12,
  kR = 0x13,
  kT = 0x14,
  kY = 0x15,
  kU = 0x16,
  kI = 0x17,
  kO = 0x18,
  kP = 0x19,
  kLeftBracket = 0x1A,
  kRightBracket = 0x1B,
  kReturn = 0x1C,  // Enter
  kLeftControl = 0x1D,
  kA = 0x1E,
  kS = 0x1F,
  kD = 0x20,
  kF = 0x21,
  kG = 0x22,
  kH = 0x23,
  kJ = 0x24,
  kK = 0x25,
  kL = 0x26,
  kSemicolon = 0x27,
  kApostrophe = 0x28,
  kGrave = 0x29,
  kLeftShift = 0x2A,
  kBackslash = 0x2B,
  kZ = 0x2C,
  kX = 0x2D,
  kC = 0x2E,
  kV = 0x2F,
  kB = 0x30,
  kN = 0x31,
  kM = 0x32,
  kComma = 0x33,
  kPreiod = 0x34,
  kSlash = 0x35,
  kRightShift = 0x36,
  /*VK_MULTIPLY*/ kNumpadMultiply = 0x37,
  kLeftAlt = 0x38,
  kSpace = 0x39,
  kCapsLock = 0x3A,
  kF1 = 0x3B,
  kF2 = 0x3C,
  kF3 = 0x3D,
  kF4 = 0x3E,
  kF5 = 0x3F,
  kF6 = 0x40,
  kF7 = 0x41,
  kF8 = 0x42,
  kF9 = 0x43,
  kF10 = 0x44,
  kNumLock = 0x45,
  kScrollLock = 0x46,
  kNumpad7 = 0x47,  // Home
  kNumpad8 = 0x48,  // Up
  kNumpad9 = 0x49,  // PageUp
  kNumpadMinus = 0x4A,
  kNumpad4 = 0x4B,  // Left
  kNumpad5 = 0x4C,
  kNumpad6 = 0x4D,  // Right
  kNumpadPlus = 0x4E,
  kNumpad1 = 0x4F,       // End
  kNumpad2 = 0x50,       // Down
  kNumpad3 = 0x51,       // PageDown
  kNumpad0 = 0x52,       // Insert
  kNumpadPeriod = 0x53,  // Delete
  kAltPrintScreen = 0x54,
  /*
   * MapVirtualKeyEx(VK_SNAPSHOT, MAPVK_VK_TO_VSC_EX, 0) returns scancode 0x54.
   */
  kBracketAngle = 0x56, /* VK_OEM_102, Key between the left shift and Z. */
  kF11 = 0x57,
  kF12 = 0x58,

  kOem1 = 0x5A,      /* VK_OEM_WSCTRL */
  kOemFinish = 0x5B, /* VK_OEM_FINISH */
  kOemJump = 0x5C,   /* VK_OEM_JUMP */
  kEraseEOF = 0x5D,  /* VK_EREOF */
  kOem4 = 0x5E,      /* VK_OEM_BACKTAB */
  kOem5 = 0x5F,      /* VK_OEM_AUTO */
  kZoom = 0x62,
  kHelp = 0x63,
  kF13 = 0x64,
  kF14 = 0x65,
  kF15 = 0x66,
  kF16 = 0x67,
  kF17 = 0x68,
  kF18 = 0x69,
  kF19 = 0x6A,
  kF20 = 0x6B,
  kF21 = 0x6C,
  kF22 = 0x6D,
  kF23 = 0x6E,
  kOem6 = 0x6F,       /* VK_OEM_PA3 */
  kKatakana = 0x70,   /* SDL_SCANCODE_INTERNATIONAL2 */
  kOem7 = 0x71,       /* VK_OEM_RESET */
  kReserved1 = 0x73,  // ?
  kF24 = 0x76,
  kSbcsChar = 0x77,
  kDbcsChar = 0x78,  // ?
  kConvert = 0x79,
  kNonConvert = 0x7B, /* VK_OEM_PA1 */
  kUnkown1 = 0x7D,    // ?
  kReserved2 = 0x7E,  /*SDL_SCANCODE_INTERNATIONAL3*/
  kMediaPrevious = 0xE010,
  kMediaNext = 0xE019,
  kNumpadEnter = 0xE01C,
  kRightControl = 0xE01D,
  kVolumeMute = 0xE020,
  kLaunchApp2 = 0xE021,
  kMediaPlay = 0xE022,
  kMediaStop = 0xE024,
  kVolumeDown = 0xE02E,
  kVolumeUp = 0xE030,
  kBrowserHome = 0xE032,
  kNumpadDivide = 0xE035,
  kPrintScreen = 0xE037,
  /* kPrintScreen:
   * - make: 0xE02A 0xE037
   * - break: 0xE0B7 0xE0AA
   * - MapVirtualKeyEx(VK_SNAPSHOT, MAPVK_VK_TO_VSC_EX, 0) returns scancode
   * 0x54;
   * - There is no VK_KEYDOWN with VK_SNAPSHOT.
   */
  kRightAlt = 0xE038,
  kCancel = 0xE046, /* CTRL + Pause */
  kHome = 0xE047,
  kUp = 0xE048,  // Up
  kPageUp = 0xE049,
  kLeft = 0xE04B,  // Arrow
  kRight = 0xE04D,  // Arrow
  kEnd = 0xE04F,
  kDown = 0xE050,  // Down
  kPageDown = 0xE051,
  kInsert = 0xE052,
  kDelete = 0xE053,
  kLeftMeta = 0xE05B,
  kRightMeta = 0xE05C,
  kApplication = 0xE05D,
  kPower = 0xE05E,
  kSleep = 0xE05F,
  kWake = 0xE063,
  kBrowserSearch = 0xE065,
  kBrowserFavorites = 0xE066,
  kBrowserRefresh = 0xE067,
  kBrowserStop = 0xE068,
  kBrowserForward = 0xE069,
  kBrowserBack = 0xE06A,
  kLaunchApp1 = 0xE06B,
  kLaunchEmail = 0xE06C,
  kLaunchMedia = 0xE06D,
  kPause = 0xE11D,  // ?
  /*
   * kPause:
   * - make: 0xE11D 45 0xE19D C5
   * - make in raw input: 0xE11D 0x45
   * - break: none
   * - No repeat when you hold the key down
   * - There are no break so I don't know how the key down/up is expected to
   * work. Raw input sends "keydown" and "keyup" messages, and it appears that
   * the keyup message is sent directly after the keydown message (you can't
   * hold the key down) so depending on when GetMessage or PeekMessage will
   * return messages, you may get both a keydown and keyup message "at the same
   * time". If you use VK messages most of the time you only get keydown
   * messages, but some times you get keyup messages too.
   * - when pressed at the same time as one or both control keys, generates a
   * 0xE046 (sc_cancel) and the string for that scancode is "break".
   */
};
