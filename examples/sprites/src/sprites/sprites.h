#ifndef SPRITES_H
#define SPRITES_H

#include <demo.h>
#include <imgui_overlay.h>

void initSprites(unsigned int, tornadoEffect *);
void freeSprites(tornadoEffect *);
t_canvas *renderSprites(int);
void flipSprites(int);
void vblSprites(int);

#endif
