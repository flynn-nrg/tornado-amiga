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
