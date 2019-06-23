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

#ifndef INCLUDE_GRAPHICS_H
#define INCLUDE_GRAPHICS_H

#include <stdint.h>
#include <string.h>

#include "bpl_headers.h"
#include "copper.h"

#define SPRITE_SETUP_FAST 0
#define SPRITE_SETUP_CHIP 1

typedef struct {
  int num_sprites;
  int spritesX[8];
  int spritesY[8];
  int spritesAttach[8];
  t_bpl_head *spr_data;

} sprite_options;

typedef struct graphicsOptions {
  uint32_t screenMode;
  uint32_t paddingTop;    // Allocate additiona (padding * width) bytes in the
                          // chunky buffers.
  uint32_t paddingBottom; // Same for the bottom of the screen.
  uint32_t flags;
  uint32_t tornadoOptions;
  uint32_t copperSize; // Maximum copper list length. Get this from the logs.
  uint32_t *customPal;
  uint32_t numSprites;
  uint32_t spriteSize;
  uint8_t *sprites[8];
  uint32_t spritesX[8];
  uint32_t spritesY[8];
  uint32_t spritesAttach[8];
} graphicsOptions;

graphics_t *initGraphics(graphicsOptions *);
void genPalette(unsigned int *, int);
void loadPalette(unsigned int *, int);
void loadRGB32toRGB(uint32_t *, uint32_t *);
void setSpriteControlWords(graphics_t *, graphicsOptions *, int);
void waitVBL();

#endif
