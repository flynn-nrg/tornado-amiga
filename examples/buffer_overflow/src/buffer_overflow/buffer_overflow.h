#ifndef BUFFER_OVERFLOW_H
#define BUFFER_OVERFLOW_H

#include <demo.h>
#include <imgui_overlay.h>

void initBufferOverflow(unsigned int, tornadoEffect *);
void freeBufferOverflow(tornadoEffect *);
t_canvas *renderBufferOverflow(int);
void flipBufferOverflow(int);

#endif
