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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <canvas.h>
#include <display_splash.h>
#include <memory.h>
#include <ptr_bridges.h>
#include <splash.h>
#include <tornado_settings.h>

static unsigned char *_chunky;
static t_canvas _fb = {.w = 0, .h = 0, .bypp = 1};

void display_init_splash(const char *const *splashFiles, int numFiles,
                         int sizeX, int sizeY, int offset, int depth,
                         int tornadoOptions) {
  _chunky = splash_init(splashFiles, numFiles, sizeX, sizeY, offset, depth,
                        tornadoOptions);
  if (!_chunky) {
    fprintf(stderr, "FATAL - Cannot initialise splash screen. Aborting.\n");
    exit(EXIT_FAILURE);
  }

  _fb.w = sizeX;
  _fb.h = sizeY;
  _fb.p.pix8 = _chunky;
}

t_canvas *display_get_splash() { return &_fb; }

void display_waitvbl_splash(int num_frames) {
  for (int i = 0; i < num_frames; i++) {
    splash_wait_vbl();
  }
}

void display_forefront_splash() { splash_forefront(); }

void display_set_palette_splash(void *palette) { splash_set_palette(palette); }

int display_get_palette_skip_splash(void *pal) {
  int *source = pal;
  return (((ENDI4(source[0]) >> 16) * 3 * sizeof(int)) + (2 * sizeof(int)));
}

void display_show_splash() { splash_show(); }

void display_flip_splash() { splash_swap_screens(); }

void display_end_splash(uint32_t *pal) {
  if (pal) {
    splash_fadeout(pal, 50);
  }
  splash_end();
}
