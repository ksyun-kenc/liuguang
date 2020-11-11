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

#ifdef __cplusplus
extern "C" {
#endif

int BGRAToI420(const uint8_t* src_bgra,
               int src_stride_bgra,
               volatile uint8_t* dst_y,
               int dst_stride_y,
               volatile uint8_t* dst_u,
               int dst_stride_u,
               volatile uint8_t* dst_v,
               int dst_stride_v,
               int width,
               int height);
int ARGBToI420(const uint8_t* src_argb,
               int src_stride_argb,
               volatile uint8_t* dst_y,
               int dst_stride_y,
               volatile uint8_t* dst_u,
               int dst_stride_u,
               volatile uint8_t* dst_v,
               int dst_stride_v,
               int width,
               int height);

// UMU: ABGR little endian (rgba in memory) to I420.
int ABGRToI420(const uint8_t* src_abgr,
               int src_stride_abgr,
               uint8_t* dst_y,
               int dst_stride_y,
               uint8_t* dst_u,
               int dst_stride_u,
               uint8_t* dst_v,
               int dst_stride_v,
               int width,
               int height);

#ifdef __cplusplus
}
#endif
