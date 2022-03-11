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

#include <string_view>

namespace regame {

const std::size_t kNumberOfSharedFrames = 2;
const std::size_t kNumberOfDataPointers = 8;

// Prefix_InstanceId_TextureId, InstanceId can be process ID
// std::format(kSharedTextureHandleNameFormat, instance_id, texture_id);
constexpr std::wstring_view kSharedTextureHandleNameFormat{L"Regame_{}_{}"};

#pragma warning(push)
#pragma warning(disable : 200)
struct SharedPacket {
  std::uint64_t timestamp;
  std::uint32_t type;
  std::uint32_t length;
  std::uint8_t data[0];
};

struct PackedAudioFrame {
  std::uint64_t timestamp;
  std::uint32_t linesize[kNumberOfDataPointers];
  std::uint8_t data[0];
};

struct SharedAudioFrames {
  char codec_name[16];
  std::uint32_t channels;
  std::uint32_t frame_size;
  std::uint32_t sample_bits;
  std::uint32_t sample_format;
  PackedAudioFrame frame0;
};

enum class VideoFrameType : std::uint32_t { kNone = 0, kYuv, kTexture };

struct SharedVideoFrameInfo {
  std::uint64_t timestamp;
  VideoFrameType type;
  std::uint32_t width;
  std::uint32_t height;
  std::uint32_t format;
  std::uint64_t window;  // HWND
};

struct VideoFrameStats {
  volatile uint64_t timestamp;
  struct {
    std::uint64_t preprocess;
    std::uint64_t nvenc;
    std::uint64_t wait_rgb_mapping;
    std::uint64_t rgb_mapping;
    std::uint64_t yuv_convert;
    std::uint64_t total;
  } elapsed;
  struct {
    std::uint64_t acquire_buffer_pending;
    std::uint64_t acquire_buffer_successed;
  } count;
};

struct PackedVideoYuvFrame {
  VideoFrameStats stats;
  std::uint8_t data[0];  // YUV
};

struct SharedVideoYuvFrames {
  std::uint32_t data_size;
  std::uint8_t data[0];  // PackedVideoYuvFrame
};

struct PackedVideoTextureFrame {
  VideoFrameStats stats;
  std::uint64_t instance_id;
  std::uint64_t texture_id;
};

struct SharedVideoTextureFrames {
  std::uint32_t data_size;  // sizeof(frames)
  PackedVideoTextureFrame frames[kNumberOfSharedFrames];
};
#pragma warning(pop)

constexpr std::wstring_view kAudioStartedEventName{
    L"{552C62EB-1506-4252-9B4C-D6B06B1E5BE0}"};
constexpr std::wstring_view kAudioStoppedEventName{
    L"{4D63D764-3C4B-4DAE-A3C3-BB0A53915F86}"};
constexpr std::wstring_view kSharedAudioFrameReadyEventName{
    L"{064FD645-205D-4EA8-8E3E-A7033B800E74}"};
constexpr std::wstring_view kSharedAudioFrameFileMappingName{
    L"{185A6EAB-BA8D-4FFB-BE3F-135C72A61606}"};

constexpr std::wstring_view kVideoStartedEventName{
    L"{75B053EC-7CF7-4608-961F-3D2663F3FB2D}"};
constexpr std::wstring_view kVideoStoppedEventName{
    L"{6D27BAD8-A314-4421-8CC1-1285E940631D}"};
constexpr std::wstring_view kSharedVideoFrameReadyEventName{
    L"{71BE7F93-E8A9-4072-8723-801CB673950F}"};
constexpr std::wstring_view kSharedVideoFrameInfoFileMappingName{
    L"{C6854F73-DE64-40E1-BECF-07B63EBF89A2}"};
constexpr std::wstring_view kSharedVideoYuvFramesFileMappingName{
    L"{2A1D2C0C-2D09-4430-AA3E-439E2300E1E5}"};
constexpr std::wstring_view kSharedVideoTextureFramesFileMappingName{
    L"{8FA0700E-F197-42AF-A3ED-123C2BB08F4A}"};

constexpr std::wstring_view kDoNotPresentEventName{
    L"{E7E2141B-4312-4609-BDEB-5B722CC01B96}"};

}  // namespace regame
