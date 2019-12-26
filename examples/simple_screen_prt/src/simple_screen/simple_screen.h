#ifndef SIMPLE_SCREEN_H
#define SIMPLE_SCREEN_H

#include <demo.h>
#include <imgui_overlay.h>

void initSimpleScreen(unsigned int, tornadoEffect *);
void freeSimpleScreen(tornadoEffect *);
t_canvas *renderSimpleScreen(int);
void flipSimpleScreen(int);

#endif
