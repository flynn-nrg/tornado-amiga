/*
Copyright (c) 2019, Miguel Mendez. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef DDPCM_LOWLEVEL_H
#define DDPCM_LOWLEVEL_H

#include "asmparm.h"

#ifdef __GCC_ELF__

static inline void initDDPCM_Decoder(int16_t **qtablesLeft,
                                     int16_t **qtablesRight,
                                     uint8_t *scalesLeft,
                                     uint8_t *scalesRight, uint8_t *left,
                                     uint8_t *right, uint32_t numFrames,
                                     uint32_t framesPerQTable) {
  register int16_t **_a0 __asm__("a0") = qtablesLeft;
  register int16_t **_a1 __asm__("a1") = qtablesRight;
  register uint8_t *_a2 __asm__("a2") = scalesLeft;
  register uint8_t *_a3 __asm__("a3") = scalesRight;
  register uint8_t *_a4 __asm__("a4") = left;
  register uint8_t *_a5 __asm__("a5") = right;
  register uint32_t _d0 __asm__("d0") = numFrames;
  register uint32_t _d1 __asm__("d1") = framesPerQTable;
  __asm__ volatile("jsr _initDDPCM_Decoder"
                   : "+r"(_a0), "+r"(_a1), "+r"(_a2), "+r"(_a3), "+r"(_a4),
                     "+r"(_a5), "+r"(_d0), "+r"(_d1)
                   :
                   : "cc", "memory");
}

static inline void decodeFrame_asm(uint8_t *src, int16_t *dst,
                                   int16_t *q_table, int32_t scale) {
  register uint8_t *_a0 __asm__("a0") = src;
  register int16_t *_a6 __asm__("a6") = dst;
  register int16_t *_a5 __asm__("a5") = q_table;
  register int32_t _d7 __asm__("d7") = scale;
  __asm__ volatile("jsr _decodeFrame_asm"
                   : "+r"(_a0), "+r"(_a6), "+r"(_a5), "+r"(_d7)
                   :
                   : "cc", "memory");
}

static inline void ddpcmAHIPlayerFunc(uint32_t numSamples, uint16_t *left,
                                      uint16_t *right) {
  register uint32_t _d0 __asm__("d0") = numSamples;
  register uint16_t *_a0 __asm__("a0") = left;
  register uint16_t *_a1 __asm__("a1") = right;
  __asm__ volatile("jsr _ddpcmAHIPlayerFunc"
                   : "+r"(_d0), "+r"(_a0), "+r"(_a1)
                   :
                   : "cc", "memory");
}

#else

void initDDPCM_Decoder(__ASMPARM("a0", int16_t **qtablesLeft),
                       __ASMPARM("a1", int16_t **qtablesRight),
                       __ASMPARM("a2", uint8_t *scalesLeft),
                       __ASMPARM("a3", uint8_t *scalesRight),
                       __ASMPARM("a4", uint8_t *left),
                       __ASMPARM("a5", uint8_t *right),
                       __ASMPARM("d0", uint32_t numFrames),
                       __ASMPARM("d1", uint32_t framesPerQTable));

void decodeFrame_asm(__ASMPARM("a0", uint8_t *src),
                     __ASMPARM("a6", int16_t *dst),
                     __ASMPARM("a5", int16_t *q_table),
                     __ASMPARM("d7", int32_t scale));

void ddpcmAHIPlayerFunc(__ASMPARM("d0", uint32_t numSamples),
                        __ASMPARM("a0", uint16_t *left),
                        __ASMPARM("a1", uint16_t *right));

#endif

void *getDDPCMMixRoutine16(void);

#endif
