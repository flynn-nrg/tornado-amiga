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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <devices/timer.h>

#ifdef __GCC_ELF__

/*
 * Native inline stubs for the GCC ELF toolchain.
 * These call the AmigaOS library vectors directly via inline asm,
 * bypassing VBCC's amiga.lib stubs which are ABI-incompatible.
 */

extern struct ExecBase *SysBase;

struct Device *TimerBase;
static struct IORequest timereq;
static uint32_t time_available = 0;

static inline BYTE _OpenDevice(CONST_STRPTR devName, ULONG unit,
                               struct IORequest *ioReq, ULONG flags) {
  register volatile int _d1 __asm("d1");
  register volatile int _a0 __asm("a0");
  register volatile int _a1 __asm("a1");
  register BYTE _res __asm("d0");
  register void *const _bn __asm("a6") = (void *)SysBase;
  register CONST_STRPTR _n1 __asm("a0") = devName;
  register ULONG _n2 __asm("d0") = unit;
  register struct IORequest *_n3 __asm("a1") = ioReq;
  register ULONG _n4 __asm("d1") = flags;
  __asm volatile("jsr %%a6@(-0x1bc:W)"
                 : "=r"(_res), "=r"(_d1), "=r"(_a0), "=r"(_a1)
                 : "r"(_bn), "rf"(_n1), "rf"(_n2), "rf"(_n3), "rf"(_n4)
                 : "fp0", "fp1", "cc", "memory");
  return _res;
}

static inline void _CloseDevice(struct IORequest *ioReq) {
  register volatile int _d0 __asm("d0");
  register volatile int _d1 __asm("d1");
  register volatile int _a0 __asm("a0");
  register volatile int _a1 __asm("a1");
  register void *const _bn __asm("a6") = (void *)SysBase;
  register struct IORequest *_n1 __asm("a1") = ioReq;
  __asm volatile("jsr %%a6@(-0x1c2:W)"
                 : "=r"(_d0), "=r"(_d1), "=r"(_a0), "=r"(_a1)
                 : "r"(_bn), "rf"(_n1)
                 : "fp0", "fp1", "cc", "memory");
}

static inline void _GetSysTime(struct timeval *dest) {
  register volatile int _d0 __asm("d0");
  register volatile int _d1 __asm("d1");
  register volatile int _a0 __asm("a0");
  register volatile int _a1 __asm("a1");
  register void *const _bn __asm("a6") = (void *)TimerBase;
  register struct timeval *_n1 __asm("a0") = dest;
  __asm volatile("jsr %%a6@(-0x42:W)"
                 : "=r"(_d0), "=r"(_d1), "=r"(_a0), "=r"(_a1)
                 : "r"(_bn), "rf"(_n1)
                 : "fp0", "fp1", "cc", "memory");
}

static inline void _SubTime(struct timeval *dest, const struct timeval *src) {
  register volatile int _d0 __asm("d0");
  register volatile int _d1 __asm("d1");
  register volatile int _a0 __asm("a0");
  register volatile int _a1 __asm("a1");
  register void *const _bn __asm("a6") = (void *)TimerBase;
  register struct timeval *_n1 __asm("a0") = dest;
  register const struct timeval *_n2 __asm("a1") = src;
  __asm volatile("jsr %%a6@(-0x30:W)"
                 : "=r"(_d0), "=r"(_d1), "=r"(_a0), "=r"(_a1)
                 : "r"(_bn), "rf"(_n1), "rf"(_n2)
                 : "fp0", "fp1", "cc", "memory");
}

void timeInit(void) {
  LONG error;
  error = _OpenDevice(TIMERNAME, 0, &timereq, 0);
  if (error) {
    fprintf(stderr, "FATAL - Could not open timer.device! Time services will "
                     "not be available.\n");
  } else {
    time_available = 1;
    TimerBase = timereq.io_Device;
  }
}

void timeEnd(void) {
  if (time_available) {
    _CloseDevice(&timereq);
  }
}

void timeGet(struct timeval *t) {
  if (time_available) {
    _GetSysTime(t);
  }
}

uint32_t timeDiffSec(struct timeval *t1, struct timeval *t2) {
  if (time_available) {
    _SubTime(t2, t1);
    return (uint32_t)t2->tv_secs;
  } else {
    return 0;
  }
}

#else /* VBCC / legacy GCC */

#include <clib/exec_protos.h>
#include <clib/timer_protos.h>

struct Device *TimerBase;
static struct IORequest timereq;
static uint32_t time_available = 0;

void timeInit(void) {
  LONG error;
  error = OpenDevice(TIMERNAME, 0, &timereq, 0);
  if (error) {
    fprintf(stderr, "FATAL - Could not open timer.device! Time services will "
                    "not be available.\n");
  } else {
    time_available = 1;
    TimerBase = timereq.io_Device;
  }
}

void timeEnd(void) {
  if (time_available) {
    CloseDevice(&timereq);
  }
}

void timeGet(struct timeval *t) {
  if (time_available) {
    GetSysTime(t);
  }
}

uint32_t timeDiffSec(struct timeval *t1, struct timeval *t2) {
  if (time_available) {
    SubTime(t2, t1);
    return (uint32_t)t2->tv_secs;
  } else {
    return 0;
  }
}

#endif /* __GCC_ELF__ */
