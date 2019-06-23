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
  splash_fadeout(pal, 50);
  splash_end();
}
