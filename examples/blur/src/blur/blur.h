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

#ifndef INCLUDE_BLUR_H
#define INCLUDE_BLUR_H

#include <asmparm.h>
#include <demo.h>
#include <imgui_overlay.h>

void flipBlur(int requires_forefront);
void vblBlur(int);
void initBlur(unsigned int options, tornadoEffect *effect);
void freeBlur(tornadoEffect *effect);
void tracksBlur(struct sync_device *);
void trackDataBlur(int, imguiOverlayData *);

t_canvas *renderBlurCode(int frame);
t_canvas *renderBlurGfx(int frame);
t_canvas *renderBlurMusic(int frame);
t_canvas *renderBlurCodeRocket(int);
t_canvas *renderBlurGfxRocket(int);
t_canvas *renderBlurMusicRocket(int);

#ifdef __AMIGA__

#ifdef __GCC_ELF__

static inline void blurRenderScanlineAsm(const int *p0, const int *p1,
                                         const short int *xt, const int m,
                                         int num, unsigned char *chunky) {
  register const int *_a0 __asm__("a0") = p0;
  register const int *_a1 __asm__("a1") = p1;
  register const short int *_a2 __asm__("a2") = xt;
  register int _d6 __asm__("d6") = m;
  register int _d7 __asm__("d7") = num;
  register unsigned char *_a6 __asm__("a6") = chunky;
  __asm__ volatile("jsr _blurRenderScanlineAsm"
                   : "+r"(_a0), "+r"(_a1), "+r"(_a2), "+r"(_d6), "+r"(_d7),
                     "+r"(_a6)
                   :
                   : "cc", "memory");
}

static inline void blurRenderScanlineMixAsm(const int *p0, const int *p1,
                                            const short int *xt, const int m,
                                            int num, unsigned char *chunky) {
  register const int *_a0 __asm__("a0") = p0;
  register const int *_a1 __asm__("a1") = p1;
  register const short int *_a2 __asm__("a2") = xt;
  register int _d6 __asm__("d6") = m;
  register int _d7 __asm__("d7") = num;
  register unsigned char *_a6 __asm__("a6") = chunky;
  __asm__ volatile("jsr _blurRenderScanlineMixAsm"
                   : "+r"(_a0), "+r"(_a1), "+r"(_a2), "+r"(_d6), "+r"(_d7),
                     "+r"(_a6)
                   :
                   : "cc", "memory");
}

#else

void blurRenderScanlineAsm(__ASMPARM("a0", const int *p0),
                           __ASMPARM("a1", const int *p1),
                           __ASMPARM("a2", const short int *xt),
                           __ASMPARM("d6", const int m),
                           __ASMPARM("d7", int num),
                           __ASMPARM("a6", unsigned char *chunky));
void blurRenderScanlineMixAsm(__ASMPARM("a0", const int *p0),
                              __ASMPARM("a1", const int *p1),
                              __ASMPARM("a2", const short int *xt),
                              __ASMPARM("d6", const int m),
                              __ASMPARM("d7", int num),
                              __ASMPARM("a6", unsigned char *chunky));

#endif

#endif

#endif
