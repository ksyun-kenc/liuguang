#pragma once

#include <string_view>

const size_t kNumberOfSharedFrames = 2;
const size_t kNumberOfDataPointers = 8;

#pragma warning(push)
#pragma warning(disable : 200)
struct SharedPacket {
  uint64_t timestamp;
  uint32_t type;
  uint32_t length;
  uint8_t data[0];
};

struct PackedAudioFrame {
  uint64_t timestamp;
  uint32_t linesize[kNumberOfDataPointers];
  uint8_t data[0];
};

struct SharedAudioFrames {
  char codec_name[16];
  uint32_t channels;
  uint32_t frame_size;
  uint32_t sample_bits;
  uint32_t sample_format;
  PackedAudioFrame frame0;
};

enum class VideoFrameType : uint32_t { NONE = 0, YUV, TEXTURE };

struct SharedVideoFrameInfo {
  uint64_t timestamp;
  VideoFrameType type;
  uint32_t width;
  uint32_t height;
  uint32_t format;
};

struct VideoFrameStats {
  volatile uint64_t timestamp;
  struct {
    uint64_t preprocess;
    uint64_t nvenc;
    uint64_t wait_rgb_mapping;
    uint64_t rgb_mapping;
    uint64_t yuv_convert;
    uint64_t total;
  } elapsed;
  struct {
    uint64_t acquire_buffer_pending;
    uint64_t acquire_buffer_successed;
  } count;
};

struct PackedVideoYuvFrame {
  VideoFrameStats stats;
  uint8_t data[0];  // YUV
};

struct SharedVideoYuvFrames {
  uint32_t data_size;
  uint8_t data[0];  // PackedVideoYuvFrame
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
