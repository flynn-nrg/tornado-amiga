/*
Copyright (c) 2019 Miguel Mendez
This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.
Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:
    1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software in a
product, an acknowledgment in the product documentation would be appreciated but
is not required.
    2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
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
