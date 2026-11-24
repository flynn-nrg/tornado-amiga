/*
Copyright (c) 2026, Miguel Mendez. All rights reserved.
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

#ifndef PLASMA_INNER_H_
#define PLASMA_INNER_H_

// On the Amiga the C code calls plasma_inner.s directly. On POSIX with
// TORNADO_68K_EMU, plasma.c calls it through the emulator instead, so this
// file is not used there.

#include <stdint.h>

#include "asmparm.h"

#ifdef __GCC_ELF__

static inline void renderPlasma_asm(unsigned char *chunky,
                                    const uint16_t *sine, int frame) {
  register unsigned char *_a0 __asm__("a0") = chunky;
  register const uint16_t *_a1 __asm__("a1") = sine;
  register int _d0 __asm__("d0") = frame;
  __asm__ volatile("jsr _renderPlasma_asm"
                   : "+r"(_a0), "+r"(_a1), "+r"(_d0)
                   :
                   : "d1", "cc", "memory");
}

#else

void renderPlasma_asm(__ASMPARM("a0", unsigned char *chunky),
                      __ASMPARM("a1", const uint16_t *sine),
                      __ASMPARM("d0", int frame));

#endif

#endif
