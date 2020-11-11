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

#include "sound_capturer.h"

#include <atlbase.h>

#include <avrt.h>

#include "app.hpp"

extern App g_app;

// unit: 100-nanosecond, 1s == 1000 * 1000 * 10 unit
constexpr REFERENCE_TIME kReferenceTimePerSecond = 10000000;
constexpr REFERENCE_TIME kReferenceTimePerMilliSecond = 10000;

namespace {
int64_t GetChannelLayout(const WAVEFORMATEX* wave_format) noexcept {
  assert(nullptr != wave_format);

  if (WAVE_FORMAT_EXTENSIBLE == wave_format->wFormatTag) {
    return reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wave_format)
        ->dwChannelMask;
  }
  return av_get_default_channel_layout(wave_format->nChannels);
}

AVSampleFormat GetSampleFormat(const WAVEFORMATEX* wave_format) noexcept {
  assert(nullptr != wave_format);

  switch (wave_format->wFormatTag) {
    case WAVE_FORMAT_PCM:
    pcm:
      if (16 == wave_format->wBitsPerSample) {
        return AV_SAMPLE_FMT_S16;
      } else if (32 == wave_format->wBitsPerSample) {
        return AV_SAMPLE_FMT_S32;
      } else if (64 == wave_format->wBitsPerSample) {
        return AV_SAMPLE_FMT_S64;
      }
      break;
    case WAVE_FORMAT_IEEE_FLOAT:
    ieee_float:
      if (32 == wave_format->wBitsPerSample) {
        return AV_SAMPLE_FMT_FLT;
      } else if (64 == wave_format->wBitsPerSample) {
        return AV_SAMPLE_FMT_DBL;
      }
    case WAVE_FORMAT_ALAW:
    case WAVE_FORMAT_MULAW:
      return AV_SAMPLE_FMT_U8;
    case WAVE_FORMAT_EXTENSIBLE: {
      const WAVEFORMATEXTENSIBLE* wfe =
          reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(wave_format);
      if (KSDATAFORMAT_SUBTYPE_PCM == wfe->SubFormat) {
        goto pcm;
      } else if (KSDATAFORMAT_SUBTYPE_IEEE_FLOAT == wfe->SubFormat) {
        goto ieee_float;
      }
      break;
    }
    default:
      break;
  }
  ATLTRACE2(atlTraceException, 0,
            "Format 0x%04x with %d bits per sample is not supported.",
            wave_format->wFormatTag, wave_format->wBitsPerSample);
  return AV_SAMPLE_FMT_NONE;
}
}  // namespace

SoundCapturer::SoundCapturer() noexcept {
  // ManualReset
  HANDLE ev = CreateEvent(g_app.SA(), TRUE, FALSE, nullptr);
  if (nullptr == ev) {
    ATLTRACE2(atlTraceException, 0, "CreateEvent() failed with %u\n",
              GetLastError());
    return;
  }
  stop_event_.Attach(ev);
}

SoundCapturer::~SoundCapturer() {
  Stop();
}

void SoundCapturer::Run() {
  if (!thread_.joinable()) {
    ResetEvent(stop_event_);
    thread_ = std::thread(&SoundCapturer::CaptureThread, this);
  }
}

void SoundCapturer::Stop() {
  if (nullptr != stop_event_) {
    SetEvent(stop_event_);
  }
  if (thread_.joinable()) {
    thread_.join();
  }

  shared_frame_ready_event_.Close();
  shared_frame_.Unmap();

  audio_resampler_.Free();
}

HRESULT SoundCapturer::GetAudioClient(IAudioClient** audio_client) {
  CComQIPtr<IMMDeviceEnumerator> enumerator;
  HRESULT hr = enumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator),
                                           nullptr, CLSCTX_ALL);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "CoCreateInstance() failed with 0x%08x\n",
              hr);
    return hr;
  }

  CComQIPtr<IMMDevice> device;
  hr = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "GetDefaultAudioEndpoint() failed with 0x%08x\n", hr);
    return hr;
  }

  hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                        reinterpret_cast<void**>(audio_client));
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "IMMDevice::Activate() failed with 0x%08x\n", hr);
  }
  return hr;
}

HRESULT SoundCapturer::GetAudioInfo(AudioInfo* info) {
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "CoInitializeEx() failed with 0x%08x\n",
              hr);
    return hr;
  }
  BOOST_SCOPE_EXIT_ALL(&) { CoUninitialize(); };

  CComQIPtr<IAudioClient> audio_client;
  hr = GetAudioClient(&audio_client);
  if (FAILED(hr)) {
    return hr;
  }

  CComHeapPtr<WAVEFORMATEX> wave_format;
  hr = audio_client->GetMixFormat(&wave_format);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "IAudioClient::GetMixFormat() failed with 0x%08x\n", hr);
    return hr;
  }

  info->sample_rate = wave_format->nSamplesPerSec;
  info->sample_bits = wave_format->wBitsPerSample;
  info->channel_layout = GetChannelLayout(wave_format);
  info->channels = wave_format->nChannels;
  info->sample_format = GetSampleFormat(wave_format);
  return hr;
}

HRESULT SoundCapturer::CaptureThread() {
  BOOST_SCOPE_EXIT_ALL(&) { SetEvent(stop_event_); };

  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0, "CoInitializeEx() failed with 0x%08x\n",
              hr);
    return hr;
  }
  BOOST_SCOPE_EXIT_ALL(&) { CoUninitialize(); };

  CComQIPtr<IAudioClient> audio_client;
  hr = GetAudioClient(&audio_client);
  if (FAILED(hr)) {
    return hr;
  }

  CComHeapPtr<WAVEFORMATEX> wave_format;
  hr = audio_client->GetMixFormat(&wave_format);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "IAudioClient::GetMixFormat() failed with 0x%08x\n", hr);
    return hr;
  }

  REFERENCE_TIME default_period = 0;
  hr = audio_client->GetDevicePeriod(&default_period, nullptr);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "IAudioClient::GetDevicePeriod() failed with 0x%08x\n", hr);
    return hr;
  }
  ATLTRACE2(atlTraceUtil, 0, "default_period = %Id\n", default_period);

  hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                AUDCLNT_STREAMFLAGS_LOOPBACK, default_period, 0,
                                wave_format, nullptr);
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "IAudioClient::Initialize() failed with 0x%08x\n", hr);
    return hr;
  }

  CComQIPtr<IAudioCaptureClient> audio_capture_client;
  hr =
      audio_client->GetService(__uuidof(IAudioCaptureClient),
                               reinterpret_cast<void**>(&audio_capture_client));
  if (FAILED(hr)) {
    ATLTRACE2(
        atlTraceException, 0,
        "IAudioClient::GetService(IAudioCaptureClient) failed with 0x%08x\n",
        hr);
    return hr;
  }

  DWORD task_index = 0;
  HANDLE task = AvSetMmThreadCharacteristics(L"Audio", &task_index);
  if (nullptr == task) {
    DWORD error_code = GetLastError();
    ATLTRACE2(atlTraceException, 0,
              "AvSetMmThreadCharacteristics() failed with %u\n", error_code);
    return HRESULT_FROM_WIN32(error_code);
  }
  BOOST_SCOPE_EXIT_ALL(&) {
    if (!AvRevertMmThreadCharacteristics(task)) {
      ATLTRACE2(atlTraceException, 0,
                "AvRevertMmThreadCharacteristics() failed with %u\n",
                GetLastError());
    }
  };

  const LONG time_between_fires = static_cast<LONG>(
      default_period /
      kReferenceTimePerMilliSecond);  // convert to milliseconds
  ATLTRACE2(atlTraceUtil, 0, "time_between_fires = %d\n", time_between_fires);

  assert(0 != out_channel_layout_);
  assert(AV_SAMPLE_FMT_NONE != out_sample_format_);
  assert(0 != out_sample_rate_);
  int error_code = audio_resampler_.Init(
      GetChannelLayout(wave_format), GetSampleFormat(wave_format),
      wave_format->nSamplesPerSec, out_channel_layout_, out_sample_format_,
      out_sample_rate_, frame_size_);
  if (error_code < 0) {
    std::cerr << "Initialize resampler failed with " << error_code << ".\n";
    return error_code;
  }

  hr = shared_frame_.OpenMapping(kSharedAudioFrameFileMappingName.data(), 0);
  if (FAILED(hr)) {
    std::cerr << "OpenMapping() failed with " << hr << ".\n";
    return hr;
  }
  SharedAudioFrames* saf = shared_frame_;
  if (saf->channels != av_get_channel_layout_nb_channels(out_channel_layout_) ||
      saf->frame_size != frame_size_ ||
      saf->sample_format != out_sample_format_) {
    std::cerr << "Invalid audio frame format.\n";
    return -1;
  }
  HANDLE ev = OpenEvent(EVENT_MODIFY_STATE, FALSE,
                        kSharedAudioFrameReadyEventName.data());
  shared_frame_ready_event_.Attach(ev);

  UINT32 silent_samples =
      wave_format->nSamplesPerSec * time_between_fires / 1000;
  std::unique_ptr<BYTE[]> silent_data =
      std::make_unique<BYTE[]>(silent_samples * wave_format->nBlockAlign);

  hr = audio_client->Start();
  if (FAILED(hr)) {
    ATLTRACE2(atlTraceException, 0,
              "IAudioClient::Start() failed with 0x%08x\n", hr);
    return hr;
  }
  BOOST_SCOPE_EXIT_ALL(&) {
    HRESULT hr = audio_client->Stop();
    if (FAILED(hr)) {
      ATLTRACE2(atlTraceException, 0,
                "IAudioClient::Stop() failed with 0x%08x\n", hr);
    }
  };

  bool is_first_packet = true;
  for (UINT32 passes = 0;; ++passes) {
    for (UINT32 next_packet_size = 0;;) {
      hr = audio_capture_client->GetNextPacketSize(&next_packet_size);
      if (FAILED(hr)) {
        ATLTRACE2(atlTraceException, 0,
                  "IAudioCaptureClient::GetNextPacketSize() failed on pass %u "
                  "after %u frames: hr = 0x%08x",
                  passes, frames_, hr);
        return hr;
      }
      if (0 == next_packet_size) {
        // copy silent
        // audio_resampler_.Store(silent_data.get(), silent_samples);
        // if (audio_resampler_.HasFrame()) {
        //  SetEvent(frame_ready_event_);
        //}
        break;
      }

      // get the captured data
      bool ok = true;
      BYTE* data = nullptr;
      UINT32 frames_to_read = 0;
      DWORD flags = 0;
      UINT64 device_position = 0;
      UINT64 qpc_position = 0;
      hr = audio_capture_client->GetBuffer(&data, &frames_to_read, &flags,
                                           &device_position, &qpc_position);
      if (FAILED(hr)) {
        // AUDCLNT_E_OUT_OF_ORDER
        ATLTRACE2(atlTraceException, 0,
                  "IAudioCaptureClient::GetBuffer() failed on pass %u after %u "
                  "frames: hr = 0x%08x\n",
                  passes, frames_, hr);
        return hr;
      }

      if (AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY & flags) {
        if (!is_first_packet) {
          ok = false;
        }
      }
      if (AUDCLNT_BUFFERFLAGS_SILENT & flags) {
        //
      }

      if (0 == frames_to_read) {
        ATLTRACE2(atlTraceException, 0,
                  "IAudioCaptureClient::GetBuffer() said to read 0 frames on "
                  "pass %u after %u frames.\n",
                  passes, frames_);
        return E_UNEXPECTED;
      }

      if (ok) {
        int new_size = 0;
        audio_resampler_.Store(data, frames_to_read, &new_size);
        if (new_size > frame_size_) {
          // Map
          SetEvent(shared_frame_ready_event_);
        }
      }

      frames_ += frames_to_read;
      ATLTRACE2(atlTraceUtil, 0,
                "Position: %Iu, %Iu, flags %d, Data: %d * %d\n",
                device_position, qpc_position, flags, frames_to_read,
                wave_format->nBlockAlign);

      hr = audio_capture_client->ReleaseBuffer(frames_to_read);
      if (FAILED(hr)) {
        ATLTRACE2(atlTraceException, 0,
                  "IAudioCaptureClient::ReleaseBuffer() failed on pass %u "
                  "after %u frames: hr = 0x%08X.\n",
                  passes, frames_, hr);
        return hr;
      }

      is_first_packet = false;
    }  // end of for

    DWORD wait_result = WaitForSingleObject(stop_event_, time_between_fires);
    if (WAIT_OBJECT_0 == wait_result) {
      std::cout << "Received stop event after " << passes << " passes and "
                << frames_ << " frames\n";
      break;
    } else if (WAIT_TIMEOUT != wait_result) {
      std::cout << "Unexpected WaitForMultipleObjects() return " << wait_result
                << " on pass " << passes << " passes and " << frames_
                << " frames\n";
      return E_UNEXPECTED;
    }
  }  // capture loop

  return S_OK;
}
