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

#ifndef INCLUDE_SYSTEM_H
#define INCLUDE_SYSTEM_H

#include "asmparm.h"

int installLevel3(__ASMPARM("a0", int *vectorBase),
                  __ASMPARM("a1", int *paulaOutputVBLCallback),
                  __ASMPARM("a2", int *optionalVBLCallback));

int installLevel2(__ASMPARM("a0", int *vectorBase));
int closeOS(__ASMPARM("a0", int *vectorBase));
int restoreOS(__ASMPARM("a0", int *vectorBase));
void serialPutc(__ASMPARM("d0", char c));

int mousePressL(void);
int mousePressR(void);
int mousePosX(void);
int mousePosY(void);
int getKeyPress(void);
void ciab_start(void);
int ciab_stop(void);
unsigned int getMasterTimer(void);
void resetMasterTimer();

extern unsigned int master_timer;
extern unsigned int mouse_left;
extern unsigned int mouse_right;

#endif
