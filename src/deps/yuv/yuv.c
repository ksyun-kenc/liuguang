/*
Copyright 2011 The LibYuv Project Authors. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.

  * Neither the name of Google nor the names of its contributors may
    be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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

// clang -O3 -c -Wall -o yuv.o yuv.c
// llvm-ar rc yuv.lib yuv.o

#include <stdint.h>
#include <string.h>

#define SIMD_ALIGNED(var) var __attribute__((aligned(16)))
typedef int16_t __attribute__((vector_size(16))) vec16;
typedef int32_t __attribute__((vector_size(16))) vec32;
typedef int8_t __attribute__((vector_size(16))) vec8;
typedef uint16_t __attribute__((vector_size(16))) uvec16;
typedef uint32_t __attribute__((vector_size(16))) uvec32;
typedef uint8_t __attribute__((vector_size(16))) uvec8;
typedef int16_t __attribute__((vector_size(32))) lvec16;
typedef int32_t __attribute__((vector_size(32))) lvec32;
typedef int8_t __attribute__((vector_size(32))) lvec8;
typedef uint16_t __attribute__((vector_size(32))) ulvec16;
typedef uint32_t __attribute__((vector_size(32))) ulvec32;
typedef uint8_t __attribute__((vector_size(32))) ulvec8;

#define IS_ALIGNED(p, a) (!((uintptr_t)(p) & ((a)-1)))
#define LABELALIGN
static const uvec8 kBGRAToY = {0u, 66u, 129u, 25u, 0u, 66u, 129u, 25u,
                               0u, 66u, 129u, 25u, 0u, 66u, 129u, 25u};

static const vec8 kBGRAToU = {0, -38, -74, 112, 0, -38, -74, 112,
                              0, -38, -74, 112, 0, -38, -74, 112};

static const vec8 kBGRAToV = {0, 112, -94, -18, 0, 112, -94, -18,
                              0, 112, -94, -18, 0, 112, -94, -18};

static const uvec16 kAddY16 = {0x7e80u, 0x7e80u, 0x7e80u, 0x7e80u,
                               0x7e80u, 0x7e80u, 0x7e80u, 0x7e80u};
static const uvec8 kAddUV128 = {128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u,
                                128u, 128u, 128u, 128u, 128u, 128u, 128u, 128u};
static const uvec16 kSub128 = {0x8080u, 0x8080u, 0x8080u, 0x8080u,
                               0x8080u, 0x8080u, 0x8080u, 0x8080u};

// TODO(mraptis): Consider passing R, G, B multipliers as parameter.
// round parameter is register containing value to add before shift.
#define RGBTOY(round)                            \
  "1:                                        \n" \
  "movdqu    (%0),%%xmm0                     \n" \
  "movdqu    0x10(%0),%%xmm1                 \n" \
  "movdqu    0x20(%0),%%xmm2                 \n" \
  "movdqu    0x30(%0),%%xmm3                 \n" \
  "psubb     %%xmm5,%%xmm0                   \n" \
  "psubb     %%xmm5,%%xmm1                   \n" \
  "psubb     %%xmm5,%%xmm2                   \n" \
  "psubb     %%xmm5,%%xmm3                   \n" \
  "movdqu    %%xmm4,%%xmm6                   \n" \
  "pmaddubsw %%xmm0,%%xmm6                   \n" \
  "movdqu    %%xmm4,%%xmm0                   \n" \
  "pmaddubsw %%xmm1,%%xmm0                   \n" \
  "movdqu    %%xmm4,%%xmm1                   \n" \
  "pmaddubsw %%xmm2,%%xmm1                   \n" \
  "movdqu    %%xmm4,%%xmm2                   \n" \
  "pmaddubsw %%xmm3,%%xmm2                   \n" \
  "lea       0x40(%0),%0                     \n" \
  "phaddw    %%xmm0,%%xmm6                   \n" \
  "phaddw    %%xmm2,%%xmm1                   \n" \
  "paddw     %%" #round                          \
  ",%%xmm6             \n"                       \
  "paddw     %%" #round                          \
  ",%%xmm1             \n"                       \
  "psrlw     $0x8,%%xmm6                     \n" \
  "psrlw     $0x8,%%xmm1                     \n" \
  "packuswb  %%xmm1,%%xmm6                   \n" \
  "movdqu    %%xmm6,(%1)                     \n" \
  "lea       0x10(%1),%1                     \n" \
  "sub       $0x10,%2                        \n" \
  "jg        1b                              \n"

void BGRAToUVRow_SSSE3(const uint8_t* src_bgra0,
                       int src_stride_bgra,
                       uint8_t* dst_u,
                       uint8_t* dst_v,
                       int width) {
  asm volatile(
      "movdqa    %5,%%xmm3                       \n"
      "movdqa    %6,%%xmm4                       \n"
      "movdqa    %7,%%xmm5                       \n"
      "sub       %1,%2                           \n"

      LABELALIGN
      "1:                                        \n"
      "movdqu    (%0),%%xmm0                     \n"
      "movdqu    0x00(%0,%4,1),%%xmm7            \n"
      "pavgb     %%xmm7,%%xmm0                   \n"
      "movdqu    0x10(%0),%%xmm1                 \n"
      "movdqu    0x10(%0,%4,1),%%xmm7            \n"
      "pavgb     %%xmm7,%%xmm1                   \n"
      "movdqu    0x20(%0),%%xmm2                 \n"
      "movdqu    0x20(%0,%4,1),%%xmm7            \n"
      "pavgb     %%xmm7,%%xmm2                   \n"
      "movdqu    0x30(%0),%%xmm6                 \n"
      "movdqu    0x30(%0,%4,1),%%xmm7            \n"
      "pavgb     %%xmm7,%%xmm6                   \n"

      "lea       0x40(%0),%0                     \n"
      "movdqa    %%xmm0,%%xmm7                   \n"
      "shufps    $0x88,%%xmm1,%%xmm0             \n"
      "shufps    $0xdd,%%xmm1,%%xmm7             \n"
      "pavgb     %%xmm7,%%xmm0                   \n"
      "movdqa    %%xmm2,%%xmm7                   \n"
      "shufps    $0x88,%%xmm6,%%xmm2             \n"
      "shufps    $0xdd,%%xmm6,%%xmm7             \n"
      "pavgb     %%xmm7,%%xmm2                   \n"
      "movdqa    %%xmm0,%%xmm1                   \n"
      "movdqa    %%xmm2,%%xmm6                   \n"
      "pmaddubsw %%xmm4,%%xmm0                   \n"
      "pmaddubsw %%xmm4,%%xmm2                   \n"
      "pmaddubsw %%xmm3,%%xmm1                   \n"
      "pmaddubsw %%xmm3,%%xmm6                   \n"
      "phaddw    %%xmm2,%%xmm0                   \n"
      "phaddw    %%xmm6,%%xmm1                   \n"
      "psraw     $0x8,%%xmm0                     \n"
      "psraw     $0x8,%%xmm1                     \n"
      "packsswb  %%xmm1,%%xmm0                   \n"
      "paddb     %%xmm5,%%xmm0                   \n"
      "movlps    %%xmm0,(%1)                     \n"
      "movhps    %%xmm0,0x00(%1,%2,1)            \n"
      "lea       0x8(%1),%1                      \n"
      "sub       $0x10,%3                        \n"
      "jg        1b                              \n"
      : "+r"(src_bgra0),                   // %0
        "+r"(dst_u),                       // %1
        "+r"(dst_v),                       // %2
        "+rm"(width)                       // %3
      : "r"((intptr_t)(src_stride_bgra)),  // %4
        "m"(kBGRAToV),                     // %5
        "m"(kBGRAToU),                     // %6
        "m"(kAddUV128)                     // %7
      : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm6", "xmm7");
}

void BGRAToYRow_SSSE3(const uint8_t* src_bgra, uint8_t* dst_y, int width) {
  asm volatile(
      "movdqa    %3,%%xmm4                       \n"
      "movdqa    %4,%%xmm5                       \n"
      "movdqa    %5,%%xmm7                       \n"

      LABELALIGN RGBTOY(xmm7)
      : "+r"(src_bgra),  // %0
        "+r"(dst_y),     // %1
        "+r"(width)      // %2
      : "m"(kBGRAToY),   // %3
        "m"(kSub128),    // %4
        "m"(kAddY16)     // %5
      : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
        "xmm7");
}
#define SS(width, shift) (((width) + (1 << (shift)) - 1) >> (shift))

#define ANY12S(NAMEANY, ANY_SIMD, UVSHIFT, BPP, MASK)                        \
  void NAMEANY(const uint8_t* src_ptr, int src_stride_ptr, uint8_t* dst_u,   \
               uint8_t* dst_v, int width) {                                  \
    SIMD_ALIGNED(uint8_t temp[128 * 4]);                                     \
    memset(temp, 0, 128 * 2); /* for msan */                                 \
    int r = width & MASK;                                                    \
    int n = width & ~MASK;                                                   \
    if (n > 0) {                                                             \
      ANY_SIMD(src_ptr, src_stride_ptr, dst_u, dst_v, n);                    \
    }                                                                        \
    memcpy(temp, src_ptr + (n >> UVSHIFT) * BPP, SS(r, UVSHIFT) * BPP);      \
    memcpy(temp + 128, src_ptr + src_stride_ptr + (n >> UVSHIFT) * BPP,      \
           SS(r, UVSHIFT) * BPP);                                            \
    if ((width & 1) && UVSHIFT == 0) { /* repeat last pixel for subsample */ \
      memcpy(temp + SS(r, UVSHIFT) * BPP, temp + SS(r, UVSHIFT) * BPP - BPP, \
             BPP);                                                           \
      memcpy(temp + 128 + SS(r, UVSHIFT) * BPP,                              \
             temp + 128 + SS(r, UVSHIFT) * BPP - BPP, BPP);                  \
    }                                                                        \
    ANY_SIMD(temp, 128, temp + 256, temp + 384, MASK + 1);                   \
    memcpy(dst_u + (n >> 1), temp + 256, SS(r, 1));                          \
    memcpy(dst_v + (n >> 1), temp + 384, SS(r, 1));                          \
  }

#define ANY11(NAMEANY, ANY_SIMD, UVSHIFT, SBPP, BPP, MASK)                \
  void NAMEANY(const uint8_t* src_ptr, uint8_t* dst_ptr, int width) {     \
    SIMD_ALIGNED(uint8_t temp[128 * 2]);                                  \
    memset(temp, 0, 128); /* for YUY2 and msan */                         \
    int r = width & MASK;                                                 \
    int n = width & ~MASK;                                                \
    if (n > 0) {                                                          \
      ANY_SIMD(src_ptr, dst_ptr, n);                                      \
    }                                                                     \
    memcpy(temp, src_ptr + (n >> UVSHIFT) * SBPP, SS(r, UVSHIFT) * SBPP); \
    ANY_SIMD(temp, temp + 128, MASK + 1);                                 \
    memcpy(dst_ptr + n * BPP, temp + 128, r * BPP);                       \
  }

ANY11(BGRAToYRow_Any_SSSE3, BGRAToYRow_SSSE3, 0, 4, 1, 15)
ANY12S(BGRAToUVRow_Any_SSSE3, BGRAToUVRow_SSSE3, 0, 4, 15)

int BGRAToI420(const uint8_t* src_bgra,
               int src_stride_bgra,
               uint8_t* dst_y,
               int dst_stride_y,
               uint8_t* dst_u,
               int dst_stride_u,
               uint8_t* dst_v,
               int dst_stride_v,
               int width,
               int height) {
  int y;
  void (*BGRAToUVRow)(const uint8_t* src_bgra0, int src_stride_bgra,
                      uint8_t* dst_u, uint8_t* dst_v, int width);
  void (*BGRAToYRow)(const uint8_t* src_bgra, uint8_t* dst_y, int width);
  if (!src_bgra || !dst_y || !dst_u || !dst_v || width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_bgra = src_bgra + (height - 1) * src_stride_bgra;
    src_stride_bgra = -src_stride_bgra;
  }

  BGRAToUVRow = BGRAToUVRow_Any_SSSE3;
  BGRAToYRow = BGRAToYRow_Any_SSSE3;
  if (IS_ALIGNED(width, 16)) {
    BGRAToUVRow = BGRAToUVRow_SSSE3;
    BGRAToYRow = BGRAToYRow_SSSE3;
  }

  for (y = 0; y < height - 1; y += 2) {
    BGRAToUVRow(src_bgra, src_stride_bgra, dst_u, dst_v, width);
    BGRAToYRow(src_bgra, dst_y, width);
    BGRAToYRow(src_bgra + src_stride_bgra, dst_y + dst_stride_y, width);
    src_bgra += src_stride_bgra * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    BGRAToUVRow(src_bgra, 0, dst_u, dst_v, width);
    BGRAToYRow(src_bgra, dst_y, width);
  }
  return 0;
}

// Constants for ARGB
static const uvec8 kARGBToY = {25u, 129u, 66u, 0u, 25u, 129u, 66u, 0u,
                               25u, 129u, 66u, 0u, 25u, 129u, 66u, 0u};

static const vec8 kARGBToU = {112, -74, -38, 0, 112, -74, -38, 0,
                              112, -74, -38, 0, 112, -74, -38, 0};

// static const vec8 kARGBToUJ = {127, -84, -43, 0, 127, -84, -43, 0,
//                               127, -84, -43, 0, 127, -84, -43, 0};

static const vec8 kARGBToV = {-18, -94, 112, 0, -18, -94, 112, 0,
                              -18, -94, 112, 0, -18, -94, 112, 0};

// static const vec8 kARGBToVJ = {-20, -107, 127, 0, -20, -107, 127, 0,
//                               -20, -107, 127, 0, -20, -107, 127, 0};

#define RGBTOY_AVX2(round)                                       \
  "1:                                        \n"                 \
  "vmovdqu    (%0),%%ymm0                    \n"                 \
  "vmovdqu    0x20(%0),%%ymm1                \n"                 \
  "vmovdqu    0x40(%0),%%ymm2                \n"                 \
  "vmovdqu    0x60(%0),%%ymm3                \n"                 \
  "vpsubb     %%ymm5, %%ymm0, %%ymm0         \n"                 \
  "vpsubb     %%ymm5, %%ymm1, %%ymm1         \n"                 \
  "vpsubb     %%ymm5, %%ymm2, %%ymm2         \n"                 \
  "vpsubb     %%ymm5, %%ymm3, %%ymm3         \n"                 \
  "vpmaddubsw %%ymm0,%%ymm4,%%ymm0           \n"                 \
  "vpmaddubsw %%ymm1,%%ymm4,%%ymm1           \n"                 \
  "vpmaddubsw %%ymm2,%%ymm4,%%ymm2           \n"                 \
  "vpmaddubsw %%ymm3,%%ymm4,%%ymm3           \n"                 \
  "lea       0x80(%0),%0                     \n"                 \
  "vphaddw    %%ymm1,%%ymm0,%%ymm0           \n" /* mutates. */  \
  "vphaddw    %%ymm3,%%ymm2,%%ymm2           \n"                 \
  "vpaddw     %%" #round                                         \
  ",%%ymm0,%%ymm0     \n" /* Add .5 for rounding. */             \
  "vpaddw     %%" #round                                         \
  ",%%ymm2,%%ymm2     \n"                                        \
  "vpsrlw     $0x8,%%ymm0,%%ymm0             \n"                 \
  "vpsrlw     $0x8,%%ymm2,%%ymm2             \n"                 \
  "vpackuswb  %%ymm2,%%ymm0,%%ymm0           \n" /* mutates. */  \
  "vpermd     %%ymm0,%%ymm6,%%ymm0           \n" /* unmutate. */ \
  "vmovdqu    %%ymm0,(%1)                    \n"                 \
  "lea       0x20(%1),%1                     \n"                 \
  "sub       $0x20,%2                        \n"                 \
  "jg        1b                              \n"                 \
  "vzeroupper                                \n"

// vpermd for vphaddw + vpackuswb vpermd.
static const lvec32 kPermdARGBToY_AVX = {0, 4, 1, 5, 2, 6, 3, 7};

// Convert 32 ARGB pixels (128 bytes) to 32 Y values.
void ARGBToYRow_AVX2(const uint8_t* src_argb, uint8_t* dst_y, int width) {
  asm volatile(
      "vbroadcastf128 %3,%%ymm4                  \n"
      "vbroadcastf128 %4,%%ymm5                  \n"
      "vbroadcastf128 %5,%%ymm7                  \n"
      "vmovdqu    %6,%%ymm6                      \n"

      LABELALIGN RGBTOY_AVX2(ymm7)
      : "+r"(src_argb),         // %0
        "+r"(dst_y),            // %1
        "+r"(width)             // %2
      : "m"(kARGBToY),          // %3
        "m"(kSub128),           // %4
        "m"(kAddY16),           // %5
        "m"(kPermdARGBToY_AVX)  // %6
      : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
        "xmm7");
}

static const lvec8 kShufARGBToUV_AVX = {
    0, 1, 8, 9, 2, 3, 10, 11, 4, 5, 12, 13, 6, 7, 14, 15,
    0, 1, 8, 9, 2, 3, 10, 11, 4, 5, 12, 13, 6, 7, 14, 15};
void ARGBToUVRow_AVX2(const uint8_t* src_argb0,
                      int src_stride_argb,
                      uint8_t* dst_u,
                      uint8_t* dst_v,
                      int width) {
  asm volatile(
      "vbroadcastf128 %5,%%ymm5                  \n"
      "vbroadcastf128 %6,%%ymm6                  \n"
      "vbroadcastf128 %7,%%ymm7                  \n"
      "sub        %1,%2                          \n" LABELALIGN
      "1:                                        \n"
      "vmovdqu    (%0),%%ymm0                    \n"
      "vmovdqu    0x20(%0),%%ymm1                \n"
      "vmovdqu    0x40(%0),%%ymm2                \n"
      "vmovdqu    0x60(%0),%%ymm3                \n"
      "vpavgb    0x00(%0,%4,1),%%ymm0,%%ymm0     \n"
      "vpavgb    0x20(%0,%4,1),%%ymm1,%%ymm1     \n"
      "vpavgb    0x40(%0,%4,1),%%ymm2,%%ymm2     \n"
      "vpavgb    0x60(%0,%4,1),%%ymm3,%%ymm3     \n"
      "lea        0x80(%0),%0                    \n"
      "vshufps    $0x88,%%ymm1,%%ymm0,%%ymm4     \n"
      "vshufps    $0xdd,%%ymm1,%%ymm0,%%ymm0     \n"
      "vpavgb     %%ymm4,%%ymm0,%%ymm0           \n"
      "vshufps    $0x88,%%ymm3,%%ymm2,%%ymm4     \n"
      "vshufps    $0xdd,%%ymm3,%%ymm2,%%ymm2     \n"
      "vpavgb     %%ymm4,%%ymm2,%%ymm2           \n"
      "vpmaddubsw %%ymm7,%%ymm0,%%ymm1           \n"
      "vpmaddubsw %%ymm7,%%ymm2,%%ymm3           \n"
      "vpmaddubsw %%ymm6,%%ymm0,%%ymm0           \n"
      "vpmaddubsw %%ymm6,%%ymm2,%%ymm2           \n"
      "vphaddw    %%ymm3,%%ymm1,%%ymm1           \n"
      "vphaddw    %%ymm2,%%ymm0,%%ymm0           \n"
      "vpsraw     $0x8,%%ymm1,%%ymm1             \n"
      "vpsraw     $0x8,%%ymm0,%%ymm0             \n"
      "vpacksswb  %%ymm0,%%ymm1,%%ymm0           \n"
      "vpermq     $0xd8,%%ymm0,%%ymm0            \n"
      "vpshufb    %8,%%ymm0,%%ymm0               \n"
      "vpaddb     %%ymm5,%%ymm0,%%ymm0           \n"
      "vextractf128 $0x0,%%ymm0,(%1)             \n"
      "vextractf128 $0x1,%%ymm0,0x0(%1,%2,1)     \n"
      "lea        0x10(%1),%1                    \n"
      "sub        $0x20,%3                       \n"
      "jg         1b                             \n"
      "vzeroupper                                \n"
      : "+r"(src_argb0),                   // %0
        "+r"(dst_u),                       // %1
        "+r"(dst_v),                       // %2
        "+rm"(width)                       // %3
      : "r"((intptr_t)(src_stride_argb)),  // %4
        "m"(kAddUV128),                    // %5
        "m"(kARGBToV),                     // %6
        "m"(kARGBToU),                     // %7
        "m"(kShufARGBToUV_AVX)             // %8
      : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
        "xmm7");
}

ANY12S(ARGBToUVRow_Any_AVX2, ARGBToUVRow_AVX2, 0, 4, 31)
ANY11(ARGBToYRow_Any_AVX2, ARGBToYRow_AVX2, 0, 4, 1, 31)

static inline void ARGBtoI420_unaligned(const uint8_t* src_argb,
                                        int src_stride_argb,
                                        uint8_t* dst_y,
                                        int dst_stride_y,
                                        uint8_t* dst_u,
                                        int dst_stride_u,
                                        uint8_t* dst_v,
                                        int dst_stride_v,
                                        int width,
                                        int height) {
  int y;
  for (y = 0; y < height - 1; y += 2) {
    ARGBToUVRow_Any_AVX2(src_argb, src_stride_argb, dst_u, dst_v, width);
    ARGBToYRow_Any_AVX2(src_argb, dst_y, width);
    ARGBToYRow_Any_AVX2(src_argb + src_stride_argb, dst_y + dst_stride_y,
                        width);
    src_argb += src_stride_argb * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ARGBToUVRow_Any_AVX2(src_argb, 0, dst_u, dst_v, width);
    ARGBToYRow_Any_AVX2(src_argb, dst_y, width);
  }
}

static inline void ARGBtoI420_aligned(const uint8_t* src_argb,
                                      int src_stride_argb,
                                      uint8_t* dst_y,
                                      int dst_stride_y,
                                      uint8_t* dst_u,
                                      int dst_stride_u,
                                      uint8_t* dst_v,
                                      int dst_stride_v,
                                      int width,
                                      int height) {
  int y;
  for (y = 0; y < height - 1; y += 2) {
    ARGBToUVRow_AVX2(src_argb, src_stride_argb, dst_u, dst_v, width);
    ARGBToYRow_AVX2(src_argb, dst_y, width);
    ARGBToYRow_AVX2(src_argb + src_stride_argb, dst_y + dst_stride_y, width);
    src_argb += src_stride_argb * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ARGBToUVRow_AVX2(src_argb, 0, dst_u, dst_v, width);
    ARGBToYRow_AVX2(src_argb, dst_y, width);
  }
}

int ARGBToI420(const uint8_t* src_argb,
               int src_stride_argb,
               uint8_t* dst_y,
               int dst_stride_y,
               uint8_t* dst_u,
               int dst_stride_u,
               uint8_t* dst_v,
               int dst_stride_v,
               int width,
               int height) {
  if (!src_argb || !dst_y || !dst_u || !dst_v || width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_argb = src_argb + (height - 1) * src_stride_argb;
    src_stride_argb = -src_stride_argb;
  }

  if (IS_ALIGNED(width, 32)) {
    ARGBtoI420_aligned(src_argb, src_stride_argb, dst_y, dst_stride_y, dst_u,
                       dst_stride_u, dst_v, dst_stride_v, width, height);
  } else {
    ARGBtoI420_unaligned(src_argb, src_stride_argb, dst_y, dst_stride_y, dst_u,
                         dst_stride_u, dst_v, dst_stride_v, width, height);
  }

  return 0;
}

// UMU: ABGRToI420
// Constants for ABGR
static const uvec8 kABGRToY = {66u, 129u, 25u, 0u, 66u, 129u, 25u, 0u,
                               66u, 129u, 25u, 0u, 66u, 129u, 25u, 0u};

static const vec8 kABGRToU = {-38, -74, 112, 0, -38, -74, 112, 0,
                              -38, -74, 112, 0, -38, -74, 112, 0};

static const vec8 kABGRToV = {112, -94, -18, 0, 112, -94, -18, 0,
                              112, -94, -18, 0, 112, -94, -18, 0};

void ABGRToUVRow_SSSE3(const uint8_t* src_abgr0,
                       int src_stride_abgr,
                       uint8_t* dst_u,
                       uint8_t* dst_v,
                       int width) {
  asm volatile(
      "movdqa    %5,%%xmm3                       \n"
      "movdqa    %6,%%xmm4                       \n"
      "movdqa    %7,%%xmm5                       \n"
      "sub       %1,%2                           \n"

      LABELALIGN
      "1:                                        \n"
      "movdqu    (%0),%%xmm0                     \n"
      "movdqu    0x00(%0,%4,1),%%xmm7            \n"
      "pavgb     %%xmm7,%%xmm0                   \n"
      "movdqu    0x10(%0),%%xmm1                 \n"
      "movdqu    0x10(%0,%4,1),%%xmm7            \n"
      "pavgb     %%xmm7,%%xmm1                   \n"
      "movdqu    0x20(%0),%%xmm2                 \n"
      "movdqu    0x20(%0,%4,1),%%xmm7            \n"
      "pavgb     %%xmm7,%%xmm2                   \n"
      "movdqu    0x30(%0),%%xmm6                 \n"
      "movdqu    0x30(%0,%4,1),%%xmm7            \n"
      "pavgb     %%xmm7,%%xmm6                   \n"

      "lea       0x40(%0),%0                     \n"
      "movdqa    %%xmm0,%%xmm7                   \n"
      "shufps    $0x88,%%xmm1,%%xmm0             \n"
      "shufps    $0xdd,%%xmm1,%%xmm7             \n"
      "pavgb     %%xmm7,%%xmm0                   \n"
      "movdqa    %%xmm2,%%xmm7                   \n"
      "shufps    $0x88,%%xmm6,%%xmm2             \n"
      "shufps    $0xdd,%%xmm6,%%xmm7             \n"
      "pavgb     %%xmm7,%%xmm2                   \n"
      "movdqa    %%xmm0,%%xmm1                   \n"
      "movdqa    %%xmm2,%%xmm6                   \n"
      "pmaddubsw %%xmm4,%%xmm0                   \n"
      "pmaddubsw %%xmm4,%%xmm2                   \n"
      "pmaddubsw %%xmm3,%%xmm1                   \n"
      "pmaddubsw %%xmm3,%%xmm6                   \n"
      "phaddw    %%xmm2,%%xmm0                   \n"
      "phaddw    %%xmm6,%%xmm1                   \n"
      "psraw     $0x8,%%xmm0                     \n"
      "psraw     $0x8,%%xmm1                     \n"
      "packsswb  %%xmm1,%%xmm0                   \n"
      "paddb     %%xmm5,%%xmm0                   \n"
      "movlps    %%xmm0,(%1)                     \n"
      "movhps    %%xmm0,0x00(%1,%2,1)            \n"
      "lea       0x8(%1),%1                      \n"
      "sub       $0x10,%3                        \n"
      "jg        1b                              \n"
      : "+r"(src_abgr0),                   // %0
        "+r"(dst_u),                       // %1
        "+r"(dst_v),                       // %2
        "+rm"(width)                       // %3
      : "r"((intptr_t)(src_stride_abgr)),  // %4
        "m"(kABGRToV),                     // %5
        "m"(kABGRToU),                     // %6
        "m"(kAddUV128)                     // %7
      : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm6", "xmm7");
}

void ABGRToYRow_SSSE3(const uint8_t* src_abgr, uint8_t* dst_y, int width) {
  asm volatile(
      "movdqa    %3,%%xmm4                       \n"
      "movdqa    %4,%%xmm5                       \n"
      "movdqa    %5,%%xmm7                       \n"

      LABELALIGN RGBTOY(xmm7)
      : "+r"(src_abgr),  // %0
        "+r"(dst_y),     // %1
        "+r"(width)      // %2
      : "m"(kABGRToY),   // %3
        "m"(kSub128),    // %4
        "m"(kAddY16)     // %5
      : "memory", "cc", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6",
        "xmm7");
}

ANY12S(ABGRToUVRow_Any_SSSE3, ABGRToUVRow_SSSE3, 0, 4, 15)
ANY11(ABGRToYRow_Any_SSSE3, ABGRToYRow_SSSE3, 0, 4, 1, 15)

int ABGRToI420(const uint8_t* src_abgr,
               int src_stride_abgr,
               uint8_t* dst_y,
               int dst_stride_y,
               uint8_t* dst_u,
               int dst_stride_u,
               uint8_t* dst_v,
               int dst_stride_v,
               int width,
               int height) {
  int y;
  void (*ABGRToUVRow)(const uint8_t* src_abgr0, int src_stride_abgr,
                      uint8_t* dst_u, uint8_t* dst_v, int width) =
      ABGRToUVRow_Any_SSSE3;
  void (*ABGRToYRow)(const uint8_t* src_abgr, uint8_t* dst_y, int width) =
      ABGRToYRow_Any_SSSE3;

  if (!src_abgr || !dst_y || !dst_u || !dst_v || width <= 0 || height == 0) {
    return -1;
  }
  // Negative height means invert the image.
  if (height < 0) {
    height = -height;
    src_abgr = src_abgr + (height - 1) * src_stride_abgr;
    src_stride_abgr = -src_stride_abgr;
  }

  if (IS_ALIGNED(width, 16)) {
    ABGRToUVRow = ABGRToUVRow_SSSE3;
    ABGRToYRow = ABGRToYRow_SSSE3;
  }

  for (y = 0; y < height - 1; y += 2) {
    ABGRToUVRow(src_abgr, src_stride_abgr, dst_u, dst_v, width);
    ABGRToYRow(src_abgr, dst_y, width);
    ABGRToYRow(src_abgr + src_stride_abgr, dst_y + dst_stride_y, width);
    src_abgr += src_stride_abgr * 2;
    dst_y += dst_stride_y * 2;
    dst_u += dst_stride_u;
    dst_v += dst_stride_v;
  }
  if (height & 1) {
    ABGRToUVRow(src_abgr, 0, dst_u, dst_v, width);
    ABGRToYRow(src_abgr, dst_y, width);
  }
  return 0;
}