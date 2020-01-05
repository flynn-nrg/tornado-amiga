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

void initDDPCM_Decoder(__ASMPARM("a0", int16_t **qtablesLeft),
                       __ASMPARM("a1", int16_t **qtablesRight),
                       __ASMPARM("a2", uint8_t *scalesLeft),
                       __ASMPARM("a3", uint8_t *scalesRight),
                       __ASMPARM("a4", uint8_t *left),
                       __ASMPARM("a5", uint8_t *right),
                       __ASMPARM("d0", uint32_t numFrames),
                       __ASMPARM("d1", uint32_t framesPerQTable));

void *getDDPCMMixRoutine16(void);

void decodeFrame_asm(__ASMPARM("a0", uint8_t *src),
                     __ASMPARM("a6", int16_t *dst),
                     __ASMPARM("a5", int16_t *q_table),
                     __ASMPARM("d7", uint8_t scale));

void ddpcmAHIPlayerFunc(__ASMPARM("d0", uint32_t numSamples),
                        __ASMPARM("a0", uint16_t *left),
                        __ASMPARM("a1", uint16_t *right));

#endif
