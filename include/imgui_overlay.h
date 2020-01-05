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

#ifndef IMGUI_OVERLAY_H_
#define IMGUI_OVERLAY_H_

#ifndef __AMIGA__

#include "SDL.h"
#include "stdint.h"

#endif

typedef struct {
  char varName[255];
  uint32_t attached;
  float min;
  float max;
  float current;
} imguiRocketSlider;

typedef struct {
  uint32_t smpte_time;
  uint32_t sliderNum;
  imguiRocketSlider *sliders;
} imguiOverlayData;

#ifndef __AMIGA__

#ifdef __cplusplus
extern "C" {
#endif

void imgui_overlay_init(SDL_Renderer *renderer, int sizex, int sizey,
                        int rocket_enable);
void imgui_overlay_close();
void imgui_overlay_set(imguiOverlayData *overlayData);
void imgui_overlay_render(void);

#ifdef __cplusplus
}
#endif

#endif // ifndef __AMIGA__

#endif // include guard
