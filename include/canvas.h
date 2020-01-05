#ifndef CANVAS_H
#define CANVAS_H

#include "memory.h"

typedef struct {
  int w, h;
  int sx, sy;
  int bypp; // bytes per pixel
  union {
    void *pixels;
    unsigned int *pix32;
    unsigned short *pix16;
    unsigned char *pix8;
  } p;
  unsigned int *pal;
} t_canvas;

static void canvas_create(t_canvas *c, int w, int h, int pixsize,
                          void *pixels) {
  c->w = w;
  c->h = h;
  c->bypp = pixsize;
  c->p.pixels = pixels;
  // If there is any palette, it's located at the end of the bitmap
  c->pal = (unsigned int *)(c->p.pix8 + c->w * c->h * c->bypp);
}

static void canvas_reserve(t_canvas *c, int w, int h, int pixsize, int npal) {
  c->w = w;
  c->h = h;
  c->bypp = pixsize;
  c->p.pixels = tndo_malloc(c->w * c->h * c->bypp, 0);
  c->pal = 0;
  if (npal != 0) {
    c->pal = (unsigned int *)tndo_malloc(npal * sizeof(int), 0);
  }
}

static void canvas_free(t_canvas *c, int w, int h, int pixsize) {}

#endif // CANVAS_H
