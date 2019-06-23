#ifndef _DEV_WINDOW_H_
#define _DEV_WINDOW_H_

#include "SDL.h"
#include "canvas.h"

#ifdef __AMIGA__
#error "don't compile this on amiga"
#endif

#define DEV_WINDOW_NORMAL 0
#define DEV_WINDOW_ROCKET 640

void dev_window_output_init(int delay, int rocketMode);

void dev_window_output_close();

void dev_window_output_flip();

t_canvas *dev_window_get_canvas();

#endif // _DEV_WINDOW_H_
