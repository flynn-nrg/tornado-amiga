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
