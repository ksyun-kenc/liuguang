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

#pragma warning(push)
#pragma warning(disable : 4819)
_CRT_BEGIN_C_HEADER

#include <libavformat/avformat.h>
#include <libavformat/avio.h>

#include <libavcodec/avcodec.h>

#include <libavutil/audio_fifo.h>
#include <libavutil/avassert.h>
#include <libavutil/avstring.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>

#include <libswresample/swresample.h>

#include <libswscale/swscale.h>

_CRT_END_C_HEADER
#pragma warning(pop)

inline const char* GetAvErrorText(const int error) {
  static char error_buffer[255];
  av_strerror(error, error_buffer, sizeof(error_buffer));
  return error_buffer;
}
