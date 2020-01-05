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

#ifndef INCLUDE_ZOOM_H
#define INCLUDE_ZOOM_H

#include <demo.h>
#include <imgui_overlay.h>

#ifndef ZOOM_TXT_SIZE
#define ZOOM_TXT_SIZE 256
#endif

#ifndef ZOOM_MIPMAPS
#define ZOOM_MIPMAPS 6
#endif

static const int mipmap_sizes[] = {256, 128, 64, 32, 16, 8, 4, 2};

typedef struct {
  int mipMapIdx;
  unsigned char ix[320];
  int iy[180];
  int iTileX[320];
  int iTileY[180];
} zoomIter;

typedef struct {
  int numKeyFrames;
  const float *kf;
} keyFrames;

void initZoom(unsigned int, tornadoEffect *);
void freeZoom(tornadoEffect *);
zoomIter *initIterations(unsigned char *, unsigned char ***, unsigned char ***,
                         unsigned char ***, unsigned char ***,
                         unsigned char ***, int, int);
t_canvas *renderZoom(int);
t_canvas *renderZoomRocket(int);
t_canvas *editZoom(int);
void saturate(unsigned char *, int, int);
void blur(unsigned char *, int, int, int);
void renderZoomC(unsigned char ***, unsigned char *, zoomIter *);
void renderClassicC(unsigned char *, unsigned char *, int);
void tracksZoom(struct sync_device *);
void trackDataZoom(int, imguiOverlayData *);
void vblZoom(int);
void flipZoom(int);

#endif
