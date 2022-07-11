/*
Copyright (c) 2019, Luis Pons. All rights reserved.

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

#include <SDL.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stb_image_write.h>

#include <canvas.h>
#include <graphics.h>
#include <imgui_overlay.h>
#include <keyboard.h>
#include <memory.h>
#include <tndo_assert.h>

// Windows are opened with a 4:3 aspect ratio to mimic a standar Amiga screen.
#define SCREEN_WIDTH (320 * 2)
#define SCREEN_HEIGHT (256 * 2)
#define SMPTE_HEIGHT 60

// The window we'll be rendering to
static SDL_Window *gWindow = NULL;

// The window renderer
static SDL_Renderer *gRenderer = NULL;
static SDL_Texture *gTexture = NULL;

static t_canvas _framebuffer = {
    .w = SCREEN_WIDTH, .h = SCREEN_HEIGHT, .bypp = 4};

static int _success = 0, _freed = 0;
static int _screen_dump_enable = 0;

typedef struct {
  int ksdl, kamiga;
} t_keytrans;

static const t_keytrans _keytrans[] = {
    {SDL_SCANCODE_ESCAPE, KEY_ESC},  {SDL_SCANCODE_SPACE, KEY_SPACE},
    {SDL_SCANCODE_F1, KEY_F1},       {SDL_SCANCODE_F2, KEY_F2},
    {SDL_SCANCODE_F3, KEY_F3},       {SDL_SCANCODE_F4, KEY_F4},
    {SDL_SCANCODE_F5, KEY_F5},       {SDL_SCANCODE_F6, KEY_F6},
    {SDL_SCANCODE_F7, KEY_F7},       {SDL_SCANCODE_F8, KEY_F8},
    {SDL_SCANCODE_F9, KEY_F9},       {SDL_SCANCODE_F10, KEY_F10},
    {SDL_SCANCODE_A, KEY_A},         {SDL_SCANCODE_B, KEY_B},
    {SDL_SCANCODE_C, KEY_C},         {SDL_SCANCODE_D, KEY_D},
    {SDL_SCANCODE_E, KEY_E},         {SDL_SCANCODE_F, KEY_F},
    {SDL_SCANCODE_G, KEY_G},         {SDL_SCANCODE_H, KEY_H},
    {SDL_SCANCODE_I, KEY_I},         {SDL_SCANCODE_J, KEY_J},
    {SDL_SCANCODE_K, KEY_K},         {SDL_SCANCODE_L, KEY_L},
    {SDL_SCANCODE_M, KEY_M},         {SDL_SCANCODE_N, KEY_N},
    {SDL_SCANCODE_O, KEY_O},         {SDL_SCANCODE_P, KEY_P},
    {SDL_SCANCODE_Q, KEY_Q},         {SDL_SCANCODE_R, KEY_R},
    {SDL_SCANCODE_S, KEY_S},         {SDL_SCANCODE_T, KEY_T},
    {SDL_SCANCODE_U, KEY_U},         {SDL_SCANCODE_V, KEY_V},
    {SDL_SCANCODE_W, KEY_W},         {SDL_SCANCODE_X, KEY_X},
    {SDL_SCANCODE_Z, KEY_Z},         {SDL_SCANCODE_RETURN, KEY_ENTER},
    {SDL_SCANCODE_UP, KEY_UP},       {SDL_SCANCODE_DOWN, KEY_DOWN},
    {SDL_SCANCODE_RIGHT, KEY_RIGHT}, {SDL_SCANCODE_LEFT, KEY_LEFT},
};

static int mouse_left = 0;
static int mouse_right = 0;
static int mouse_x = 0, mouse_y = 0;
static int flip_delay = 25;
static int modulo = 0;
static int rocket_enabled = 0;

void dev_window_output_init(int delay, int rocketMode) {
  // re-init allowed
  if (_success)
    return;

  modulo = rocketMode;

  if (rocketMode) {
    rocket_enabled = 1;
  }

  // Initialization flag
  flip_delay = delay;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    fprintf(stderr, "SDL could not initialize! SDL Error: %s\n",
            SDL_GetError());
    exit(EXIT_FAILURE);
  }

  gWindow = SDL_CreateWindow("Tornado 2 Window", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH + modulo,
                             SCREEN_HEIGHT + SMPTE_HEIGHT, SDL_WINDOW_SHOWN);
  if (gWindow == NULL) {
    fprintf(stderr, "Window could not be created! SDL Error: %s\n",
            SDL_GetError());
    exit(EXIT_FAILURE);
  }

  gRenderer = SDL_CreateRenderer(gWindow, -1, 0); // SDL_RENDERER_ACCELERATED );
  if (gRenderer == NULL) {
    fprintf(stderr, "Renderer could not be created! SDL Error: %s\n",
            SDL_GetError());
    exit(EXIT_FAILURE);
  }

  gTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH + modulo,
                               SCREEN_HEIGHT + SMPTE_HEIGHT);
  if (gTexture == NULL) {
    fprintf(stderr, "Texture could not be created! SDL Error: %s\n",
            SDL_GetError());
    exit(EXIT_FAILURE);
  }

  _framebuffer.p.pixels = (unsigned int *)malloc(
      (SCREEN_WIDTH + modulo) * (SCREEN_HEIGHT + SMPTE_HEIGHT) * sizeof(int));
  _success = 1;

  imgui_overlay_init(gRenderer, SCREEN_WIDTH + modulo,
                     SCREEN_HEIGHT + SMPTE_HEIGHT, rocket_enabled);

  tndo_assert(_success);
}

int sdl_quit = 0;

void dev_window_output_close() {
  if (_freed)
    return;

  SDL_DestroyTexture(gTexture);
  SDL_DestroyRenderer(gRenderer);
  SDL_DestroyWindow(gWindow);
  gWindow = NULL;
  gRenderer = NULL;
  free(_framebuffer.p.pixels);

  imgui_overlay_close();

  _freed = 1;
}

t_canvas *dev_window_get_canvas() { return &_framebuffer; }

static int _last_key = 0;

static void check_events() {
  SDL_Event e;
  while (SDL_PollEvent(&e) != 0) {
    // User requests quit
    if (e.type == SDL_QUIT) {
      exit(EXIT_SUCCESS);
      sdl_quit = 1;
    }

    if (e.type == SDL_MOUSEBUTTONDOWN) {
      if (e.button.button == SDL_BUTTON_LEFT)
        mouse_left = 1;
      if (e.button.button == SDL_BUTTON_RIGHT)
        mouse_right = 1;
    }
    if (e.type == SDL_MOUSEBUTTONUP) {
      if (e.button.button == SDL_BUTTON_LEFT)
        mouse_left = 0;
      if (e.button.button == SDL_BUTTON_RIGHT)
        mouse_right = 0;
    }

    if (e.type == SDL_KEYDOWN) {
      int keyboard_translation_found = 0;
      int i;
      for (i = 0; i < (int)(sizeof(_keytrans) / sizeof(t_keytrans)); i++)
        if (_keytrans[i].ksdl == e.key.keysym.scancode) {
          keyboard_translation_found = 1;
          _last_key = _keytrans[i].kamiga;
          break;
        }
    }
  }
}

static void dump_to_drive() {
  static int cnt = 0;
  char filename[20];
  sprintf(filename, "/tmp/dump%04d.bmp\n", cnt);
  filename[17] = 0;
  static int init = 0;
  static char *rgb = 0;
  if (!init)
    rgb = (char *)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 3), init = 1;
  int i;
  for (i = 0; i < (SCREEN_WIDTH * SCREEN_HEIGHT); i++) {
    rgb[i * 3 + 0] = _framebuffer.p.pix32[i] >> 16;
    rgb[i * 3 + 1] = _framebuffer.p.pix32[i] >> 8;
    rgb[i * 3 + 2] = _framebuffer.p.pix32[i] >> 0;
  }

  stbi_write_bmp(filename, SCREEN_WIDTH, SCREEN_HEIGHT, 3, rgb);
  cnt++;
}

void screen_dump_enable() { _screen_dump_enable = 1; }

void dev_window_output_flip() {
  if (_screen_dump_enable) {
    dump_to_drive();
  }

  SDL_UpdateTexture(gTexture, NULL, (const void *)_framebuffer.p.pixels,
                    (SCREEN_WIDTH + modulo) * sizeof(unsigned int));
  SDL_RenderCopy(gRenderer, gTexture, NULL, NULL);

  imgui_overlay_render();

  SDL_RenderPresent(gRenderer);

  if (!rocket_enabled) {
    check_events();
  }

  waitVBL();
}

int mousePressL(void) { return mouse_left; }

int display_checkinput(void) { return mouse_left; }

int mousePressR(void) { return mouse_right; }

int mousePosX(void) { return mouse_x; }

int mousePosY(void) { return mouse_y; }

int getKeyPress(void) {
  int cp = _last_key;
  _last_key = 0;
  return cp;
}
