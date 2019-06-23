#ifndef DISPLAY_H
#define DISPLAY_H

#include <canvas.h>
#include <graphics.h>

int display_init(unsigned int *pal, unsigned int options, int mode,
                 unsigned int padding_top, unsigned int padding_bottom,
                 sprite_options *so);
t_canvas *display_get(int);
void display_1x2_set(int);
void display_forefront(int);
void display_flip(int);
void display_end(int);
void display_set_copper(int);
void display_set_sprites_pos(int, sprite_options *);
void display_set_fade_black(int, int);
void display_set_fade_white(int, int);
void display_subsystem_init(int numInstances);
void display_subsystem_end(void);

#ifdef __AMIGA__

typedef struct {
  graphics_t *graph;
  unsigned int *pal256;
  unsigned int pal256black[16][256];
  unsigned int pal256white[16][256];
  unsigned char *display_sprites[8];
  int paddingTop;
  int paddingBottom;
  int c2pSkip;
  int numSprites;
  int sprSize;
  int mode;
  unsigned char *chunky;
  unsigned char *planar[2];
  int p;
} display_instance;

#else

typedef struct {
  unsigned int pal256[256];
  t_canvas fb;
  int mode;
  int is16_9;
  unsigned int paddingTop;
  unsigned int paddingBottom;
} display_instance;

#endif // ifdef __AMIGA__

#endif
