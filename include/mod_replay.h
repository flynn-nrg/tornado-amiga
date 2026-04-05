#ifndef MOD_REPLAY_H
#define MOD_REPLAY_H

#ifdef __AMIGA__

#include "asmparm.h"

#ifdef __GCC_ELF__

static inline int p61Init(void *module, void *external_samples,
                          void *sample_buffer, int cia_timer) {
  register void *_a0 __asm__("a0") = module;
  register void *_a1 __asm__("a1") = external_samples;
  register void *_a2 __asm__("a2") = sample_buffer;
  register int _d0 __asm__("d0") = cia_timer;
  __asm__ volatile("jsr _p61Init"
                   : "+r"(_d0), "+r"(_a0), "+r"(_a1), "+r"(_a2)
                   :
                   : "cc", "memory");
  return _d0;
}

static inline int p61InitOSLegal(void *module, void *external_samples,
                                 void *sample_buffer, int cia_timer) {
  register void *_a0 __asm__("a0") = module;
  register void *_a1 __asm__("a1") = external_samples;
  register void *_a2 __asm__("a2") = sample_buffer;
  register int _d0 __asm__("d0") = cia_timer;
  __asm__ volatile("jsr _p61InitOSLegal"
                   : "+r"(_d0), "+r"(_a0), "+r"(_a1), "+r"(_a2)
                   :
                   : "cc", "memory");
  return _d0;
}

static inline void p61SetVolOSLegal(int volume) {
  register int _d0 __asm__("d0") = volume;
  __asm__ volatile("jsr _p61SetVolOSLegal" : "+r"(_d0) : : "cc", "memory");
}

#else

int p61Init(__ASMPARM("a0", void *module),
            __ASMPARM("a1", void *external_samples),
            __ASMPARM("a2", void *sample_buffer),
            __ASMPARM("d0", int cia_timer));

int p61InitOSLegal(__ASMPARM("a0", void *module),
                   __ASMPARM("a1", void *external_samples),
                   __ASMPARM("a2", void *sample_buffer),
                   __ASMPARM("d0", int cia_timer));

void p61SetVolOSLegal(__ASMPARM("d0", int volume));

#endif

#else
int p61Init(void *module, void *external_samples, void *sample_buffer,
            int cia_timer);
#endif

void p61End(void);
void p61EndOSLegal(void);

#endif
