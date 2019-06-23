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

#ifndef INCLUDE_DEMO_H
#define INCLUDE_DEMO_H

#include <canvas.h>
#include <copper.h>
#include <debug.h>
#include <graphics.h>
#include <imgui_overlay.h>
#include <memory.h>
#include <telemetry.h>
#include <tornado_settings.h>

#include "sync.h"

typedef struct tornadoEffect {
  int minTime;
  int numAssets;
  void **Assets;
  int *assetSizes;
  int debug_color;
  int debug_pos_y;
  // Set to the # of telemetry tracks you need.
  int wantTelemetry;
  TelemetryData **telemetry;

  // Called during demoInit & demoFree.
  void (*init)(unsigned int options, struct tornadoEffect *effect);
  void (*flip)(int requires_forefront);
  void (*free)(struct tornadoEffect *effect);

  // Called once per vertical blank. The parameter has the time relative
  // to the start of the effect.
  void (*vbl)(int frame);
  // Called once on each frame to draw. The frame parameter has the time
  // relative to the start of the effect.
  t_canvas *(*render)(int frame);
  t_canvas *(*rocketRender)(int frame);
  // Rocket track initialisation.
  void (*tracks)(struct sync_device *rocket);
  // Rocket row data getter. Once per frame.
  void (*trackData)(int frame, imguiOverlayData *overlayData);

} tornadoEffect;

void demoSettings(demoParams *);
void demoInit(unsigned int, int);
void demoSplash(unsigned int);
void demoMain(unsigned int, memoryLog *);
void demoVBL(void);
void demoFree(void);

static void initNull(unsigned int options, tornadoEffect *effect) {}
static void freeNull(tornadoEffect *effect) {}
static void flipNull(int requires_forefront) {}
static void vblNull(int frame) {}
static t_canvas *renderNull(int frame) { return 0; }
static void trackDataNull(int frame, imguiOverlayData *overlayData) {}
static void tracksNull(struct sync_device *rocket) {}

#endif // INCLUDE_DEMO_H
