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
