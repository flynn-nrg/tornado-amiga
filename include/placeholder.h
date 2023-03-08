#ifndef TNDO_PLACEHOLDER_H
#define TNDO_PLACEHOLDER_H

#include <demo.h>
#include <imgui_overlay.h>

void initPlaceholder(unsigned int, tornadoEffect *);
void freePlaceholder(tornadoEffect *);
t_canvas *renderPlaceholder(int);
void flipPlaceholder(int);

#endif