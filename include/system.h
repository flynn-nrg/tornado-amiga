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

#ifdef __GCC_ELF__

static inline int installLevel3(int *vectorBase, int *paulaOutputVBLCallback,
                                int *optionalVBLCallback) {
  register int *_a0 __asm__("a0") = vectorBase;
  register int *_a1 __asm__("a1") = paulaOutputVBLCallback;
  register int *_a2 __asm__("a2") = optionalVBLCallback;
  register int _ret __asm__("d0");
  __asm__ volatile("jsr _installLevel3"
                   : "=r"(_ret), "+r"(_a0), "+r"(_a1), "+r"(_a2)
                   :
                   : "cc", "memory");
  return _ret;
}

static inline int setupVBLChain(int *paulaOutputVBLCallback,
                                int *optionalVBLCallback) {
  register int *_a1 __asm__("a1") = paulaOutputVBLCallback;
  register int *_a2 __asm__("a2") = optionalVBLCallback;
  register int _ret __asm__("d0");
  __asm__ volatile("jsr _setupVBLChain"
                   : "=r"(_ret), "+r"(_a1), "+r"(_a2)
                   :
                   : "cc", "memory");
  return _ret;
}

static inline int installLevel2(int *vectorBase) {
  register int *_a0 __asm__("a0") = vectorBase;
  register int _ret __asm__("d0");
  __asm__ volatile("jsr _installLevel2"
                   : "=r"(_ret), "+r"(_a0)
                   :
                   : "cc", "memory");
  return _ret;
}

static inline int closeOS(int *vectorBase) {
  register int *_a0 __asm__("a0") = vectorBase;
  register int _ret __asm__("d0");
  __asm__ volatile("jsr _closeOS"
                   : "=r"(_ret), "+r"(_a0)
                   :
                   : "cc", "memory");
  return _ret;
}

static inline int restoreOS(int *vectorBase) {
  register int *_a0 __asm__("a0") = vectorBase;
  register int _ret __asm__("d0");
  __asm__ volatile("jsr _restoreOS"
                   : "=r"(_ret), "+r"(_a0)
                   :
                   : "cc", "memory");
  return _ret;
}

static inline void serialPutc(char c) {
  register char _d0 __asm__("d0") = c;
  __asm__ volatile("jsr _serialPutc" : "+r"(_d0) : : "cc", "memory");
}

#else

int installLevel3(__ASMPARM("a0", int *vectorBase),
                  __ASMPARM("a1", int *paulaOutputVBLCallback),
                  __ASMPARM("a2", int *optionalVBLCallback));

int setupVBLChain(__ASMPARM("a1", int *paulaOutputVBLCallback),
                  __ASMPARM("a2", int *optionalVBLCallback));

int installLevel2(__ASMPARM("a0", int *vectorBase));
int closeOS(__ASMPARM("a0", int *vectorBase));
int restoreOS(__ASMPARM("a0", int *vectorBase));
void serialPutc(__ASMPARM("d0", char c));

#endif

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
