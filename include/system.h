/*
Copyright (c) 2019, Miguel Mendez. All rights reserved.

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

#ifndef INCLUDE_SYSTEM_H
#define INCLUDE_SYSTEM_H

#include "asmparm.h"

int installLevel3(__ASMPARM("a0", int *vectorBase),
                  __ASMPARM("a1", int *paulaOutputVBLCallback),
                  __ASMPARM("a2", int *optionalVBLCallback));

int setupVBLChain(__ASMPARM("a1", int *paulaOutputVBLCallback),
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
void VBLChain();

extern unsigned int master_timer;
extern unsigned int mouse_left;
extern unsigned int mouse_right;

#endif
