/*
Copyright (c) 2020, Miguel Mendez. All rights reserved.

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
#ifndef __CINTER_WRAPPER_H
#define __CINTER_WRAPPER_H

#ifdef __AMIGA__

#include "asmparm.h"

#ifdef __GCC_ELF__

static inline void CinterInit(void *music_data, void *intrument_data,
                               int instrument_data_size) {
  register void *_a2 __asm__("a2") = music_data;
  register void *_a0 __asm__("a0") = intrument_data;
  register int _d0 __asm__("d0") = instrument_data_size;
  __asm__ volatile("jsr _CinterInit"
                   : "+r"(_a2), "+r"(_a0), "+r"(_d0)
                   :
                   : "cc", "memory");
}

#else

void CinterInit(__ASMPARM("a2", void *music_data),
                __ASMPARM("a0", void *intrument_data),
                __ASMPARM("d0", int instrument_data_size));

#endif

void CinterPlay1(void);
void CinterPlay2(void);
void CinterEnd();

#else

void CinterInit(void *music_data, void *intrument_data,
                int instrument_data_size);
void CinterPlay1(void);
void CinterPlay2(void);
void CinterEnd();

void CinterInit(void *music_data, void *intrument_data,
                int instrument_data_size) {}

void Cinterplay() {}

void CinterPlay2() {}

void CinterEnd() {}

#endif // __AMIGA__

#endif // __CINTER_WRAPPER_H
