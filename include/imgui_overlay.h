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
