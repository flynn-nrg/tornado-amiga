#ifndef INCLUDE_C2P_H
#define INCLUDE_C2P_H

#include "asmparm.h"

#ifdef __GCC_ELF__

/* --- Assembly c2p wrappers for GCC 14 ELF --- */

// 8 bitplane 320x256 c2p
static inline void c2p1x1_8_c5_init(int chunkyx, int chunkyy, int scroffsx,
                                     int scroffsy, int rowlen, int bplsize) {
  register int _d0 __asm__("d0") = chunkyx;
  register int _d1 __asm__("d1") = chunkyy;
  register int _d2 __asm__("d2") = scroffsx;
  register int _d3 __asm__("d3") = scroffsy;
  register int _d4 __asm__("d4") = rowlen;
  register int _d5 __asm__("d5") = bplsize;
  __asm__ volatile("jsr _c2p1x1_8_c5_init"
                   : "+r"(_d0), "+r"(_d1), "+r"(_d2), "+r"(_d3), "+r"(_d4),
                     "+r"(_d5)
                   :
                   : "cc", "memory");
}

static inline void c2p1x1_8_c5(unsigned char *chunkyBuffer,
                                unsigned char *planes) {
  register unsigned char *_a0 __asm__("a0") = chunkyBuffer;
  register unsigned char *_a1 __asm__("a1") = planes;
  __asm__ volatile("jsr _c2p1x1_8_c5"
                   : "+r"(_a0), "+r"(_a1)
                   :
                   : "cc", "memory");
}

// 5 bitplane no-modulo c2p
static inline void c2p1x1_5_c5_060(unsigned char *chunkyBuffer,
                                    unsigned char *planes,
                                    unsigned int pixels) {
  register unsigned char *_a0 __asm__("a0") = chunkyBuffer;
  register unsigned char *_a1 __asm__("a1") = planes;
  register unsigned int _d0 __asm__("d0") = pixels;
  __asm__ volatile("jsr _c2p1x1_5_c5_060"
                   : "+r"(_a0), "+r"(_a1), "+r"(_d0)
                   :
                   : "cc", "memory");
}

// 6 bitplane RGB c2p
static inline void c2pRGB(unsigned char *chunky, unsigned char *bitplanes,
                           unsigned int w, unsigned int h,
                           unsigned int stride_chunky,
                           unsigned int stride_bpls) {
  register unsigned char *_a0 __asm__("a0") = chunky;
  register unsigned char *_a1 __asm__("a1") = bitplanes;
  register unsigned int _d0 __asm__("d0") = w;
  register unsigned int _d1 __asm__("d1") = h;
  register unsigned int _d2 __asm__("d2") = stride_chunky;
  register unsigned int _d3 __asm__("d3") = stride_bpls;
  __asm__ volatile("jsr _c2pRGB"
                   : "+r"(_a0), "+r"(_a1), "+r"(_d0), "+r"(_d1), "+r"(_d2),
                     "+r"(_d3)
                   :
                   : "cc", "memory");
}

// 6 bitplane no-modulo c2p
static inline void c2p1x1_6_c5_gen(unsigned char *chunkyBuffer,
                                    unsigned char *planes,
                                    unsigned int pixels) {
  register unsigned char *_a0 __asm__("a0") = chunkyBuffer;
  register unsigned char *_a1 __asm__("a1") = planes;
  register unsigned int _d0 __asm__("d0") = pixels;
  __asm__ volatile("jsr _c2p1x1_6_c5_gen"
                   : "+r"(_a0), "+r"(_a1), "+r"(_d0)
                   :
                   : "cc", "memory");
}

// 4 bitplane 320x180 c2p
static inline void c2p1x1_4_c5_16_9_init(int chunkyx, int chunkyy,
                                          int scroffsx, int scroffsy,
                                          int rowlen, int bplsize) {
  register int _d0 __asm__("d0") = chunkyx;
  register int _d1 __asm__("d1") = chunkyy;
  register int _d2 __asm__("d2") = scroffsx;
  register int _d3 __asm__("d3") = scroffsy;
  register int _d4 __asm__("d4") = rowlen;
  register int _d5 __asm__("d5") = bplsize;
  __asm__ volatile("jsr _c2p1x1_4_c5_16_9_init"
                   : "+r"(_d0), "+r"(_d1), "+r"(_d2), "+r"(_d3), "+r"(_d4),
                     "+r"(_d5)
                   :
                   : "cc", "memory");
}

static inline void c2p1x1_4_c5_16_9(unsigned char *chunkyBuffer,
                                     unsigned char *planes) {
  register unsigned char *_a0 __asm__("a0") = chunkyBuffer;
  register unsigned char *_a1 __asm__("a1") = planes;
  __asm__ volatile("jsr _c2p1x1_4_c5_16_9"
                   : "+r"(_a0), "+r"(_a1)
                   :
                   : "cc", "memory");
}

// 4 bitplane 640x180 c2p
static inline void c2p1x1_4_c5_16_9_h_init(int chunkyx, int chunkyy,
                                            int scroffsx, int scroffsy,
                                            int rowlen, int bplsize) {
  register int _d0 __asm__("d0") = chunkyx;
  register int _d1 __asm__("d1") = chunkyy;
  register int _d2 __asm__("d2") = scroffsx;
  register int _d3 __asm__("d3") = scroffsy;
  register int _d4 __asm__("d4") = rowlen;
  register int _d5 __asm__("d5") = bplsize;
  __asm__ volatile("jsr _c2p1x1_4_c5_16_9_h_init"
                   : "+r"(_d0), "+r"(_d1), "+r"(_d2), "+r"(_d3), "+r"(_d4),
                     "+r"(_d5)
                   :
                   : "cc", "memory");
}

static inline void c2p1x1_4_c5_16_9_h(unsigned char *chunkyBuffer,
                                       unsigned char *planes) {
  register unsigned char *_a0 __asm__("a0") = chunkyBuffer;
  register unsigned char *_a1 __asm__("a1") = planes;
  __asm__ volatile("jsr _c2p1x1_4_c5_16_9_h"
                   : "+r"(_a0), "+r"(_a1)
                   :
                   : "cc", "memory");
}

// 8 bitplane 320x256 screen size c2p
static inline void c2p1x1_8_c5_040_init(int chunkyx, int chunkyy, int scroffsx,
                                         int scroffsy, int rowlen, int bplsize,
                                         int chunkylen) {
  register int _d0 __asm__("d0") = chunkyx;
  register int _d1 __asm__("d1") = chunkyy;
  register int _d2 __asm__("d2") = scroffsx;
  register int _d3 __asm__("d3") = scroffsy;
  register int _d4 __asm__("d4") = rowlen;
  register int _d5 __asm__("d5") = bplsize;
  register int _d6 __asm__("d6") = chunkylen;
  __asm__ volatile("jsr _c2p1x1_8_c5_040_init"
                   : "+r"(_d0), "+r"(_d1), "+r"(_d2), "+r"(_d3), "+r"(_d4),
                     "+r"(_d5), "+r"(_d6)
                   :
                   : "cc", "memory");
}

static inline void c2p1x1_8_c5_040(unsigned char *chunkyBuffer,
                                    unsigned char *planes) {
  register unsigned char *_a0 __asm__("a0") = chunkyBuffer;
  register unsigned char *_a1 __asm__("a1") = planes;
  __asm__ volatile("jsr _c2p1x1_8_c5_040"
                   : "+r"(_a0), "+r"(_a1)
                   :
                   : "cc", "memory");
}

// 8 bitplane 320x1 screen size c2p
static inline void c2p1x1_8_c5_040_scanline_init(int chunkyx, int chunkyy,
                                                  int scroffsx, int scroffsy,
                                                  int rowlen, int bplsize,
                                                  int chunkylen) {
  register int _d0 __asm__("d0") = chunkyx;
  register int _d1 __asm__("d1") = chunkyy;
  register int _d2 __asm__("d2") = scroffsx;
  register int _d3 __asm__("d3") = scroffsy;
  register int _d4 __asm__("d4") = rowlen;
  register int _d5 __asm__("d5") = bplsize;
  register int _d6 __asm__("d6") = chunkylen;
  __asm__ volatile("jsr _c2p1x1_8_c5_040_scanline_init"
                   : "+r"(_d0), "+r"(_d1), "+r"(_d2), "+r"(_d3), "+r"(_d4),
                     "+r"(_d5), "+r"(_d6)
                   :
                   : "cc", "memory");
}

static inline void c2p1x1_8_c5_040_scanline(unsigned char *chunkyBuffer,
                                             unsigned char *planes) {
  register unsigned char *_a0 __asm__("a0") = chunkyBuffer;
  register unsigned char *_a1 __asm__("a1") = planes;
  __asm__ volatile("jsr _c2p1x1_8_c5_040_scanline"
                   : "+r"(_a0), "+r"(_a1)
                   :
                   : "cc", "memory");
}

// 8 bitplane 320x180 screen size c2p
static inline void c2p1x1_8_c5_040_16_9_init(int chunkyx, int chunkyy,
                                              int scroffsx, int scroffsy,
                                              int rowlen, int bplsize,
                                              int chunkylen) {
  register int _d0 __asm__("d0") = chunkyx;
  register int _d1 __asm__("d1") = chunkyy;
  register int _d2 __asm__("d2") = scroffsx;
  register int _d3 __asm__("d3") = scroffsy;
  register int _d4 __asm__("d4") = rowlen;
  register int _d5 __asm__("d5") = bplsize;
  register int _d6 __asm__("d6") = chunkylen;
  __asm__ volatile("jsr _c2p1x1_8_c5_040_16_9_init"
                   : "+r"(_d0), "+r"(_d1), "+r"(_d2), "+r"(_d3), "+r"(_d4),
                     "+r"(_d5), "+r"(_d6)
                   :
                   : "cc", "memory");
}

static inline void c2p1x1_8_c5_040_16_9(unsigned char *chunkyBuffer,
                                         unsigned char *planes) {
  register unsigned char *_a0 __asm__("a0") = chunkyBuffer;
  register unsigned char *_a1 __asm__("a1") = planes;
  __asm__ volatile("jsr _c2p1x1_8_c5_040_16_9"
                   : "+r"(_a0), "+r"(_a1)
                   :
                   : "cc", "memory");
}

// 6 bitplane arbitrary screen size c2p
static inline void c2p1x1_6_c5_040_init(int chunkyx, int chunkyy, int scroffsx,
                                         int scroffsy, int rowlen, int bplsize,
                                         int chunkylen) {
  register int _d0 __asm__("d0") = chunkyx;
  register int _d1 __asm__("d1") = chunkyy;
  register int _d2 __asm__("d2") = scroffsx;
  register int _d3 __asm__("d3") = scroffsy;
  register int _d4 __asm__("d4") = rowlen;
  register int _d5 __asm__("d5") = bplsize;
  register int _d6 __asm__("d6") = chunkylen;
  __asm__ volatile("jsr _c2p1x1_6_c5_040_init"
                   : "+r"(_d0), "+r"(_d1), "+r"(_d2), "+r"(_d3), "+r"(_d4),
                     "+r"(_d5), "+r"(_d6)
                   :
                   : "cc", "memory");
}

static inline void c2p1x1_6_c5_040(unsigned char *chunkyBuffer,
                                    unsigned char *planes) {
  register unsigned char *_a0 __asm__("a0") = chunkyBuffer;
  register unsigned char *_a1 __asm__("a1") = planes;
  __asm__ volatile("jsr _c2p1x1_6_c5_040"
                   : "+r"(_a0), "+r"(_a1)
                   :
                   : "cc", "memory");
}

// 6 bitplane c2p
static inline void c2p64(unsigned char *chunky, unsigned char *chunky_end,
                          unsigned char *bpls) {
  register unsigned char *_a0 __asm__("a0") = chunky;
  register unsigned char *_a1 __asm__("a1") = chunky_end;
  register unsigned char *_a2 __asm__("a2") = bpls;
  __asm__ volatile("jsr _c2p64"
                   : "+r"(_a0), "+r"(_a1), "+r"(_a2)
                   :
                   : "cc", "memory");
}

#ifdef __AMIGA__
// 1x1 8bpl cpu5 C2P for [almost] arbitrary BitMaps
static inline void c2p1x1_8_c5_bm(char *chunkybuffer, struct BitMap *bitmap,
                                   int chunkyxsize, int chunkyysize,
                                   int xoffset, int yoffset) {
  register char *_a0 __asm__("a0") = chunkybuffer;
  register struct BitMap *_a1 __asm__("a1") = bitmap;
  register int _d0 __asm__("d0") = chunkyxsize;
  register int _d1 __asm__("d1") = chunkyysize;
  register int _d2 __asm__("d2") = xoffset;
  register int _d3 __asm__("d3") = yoffset;
  __asm__ volatile("jsr _c2p1x1_8_c5_bm"
                   : "+r"(_a0), "+r"(_a1), "+r"(_d0), "+r"(_d1), "+r"(_d2),
                     "+r"(_d3)
                   :
                   : "cc", "memory");
}
#endif

// c2p_8bpl_scanline is implemented in C (src/c2p.c) -- standard calling convention.
void c2p_8bpl_scanline(
    __ASMPARM("d0", int pixels), __ASMPARM("a0", uint32_t *chunky),
    __ASMPARM("a1", uint32_t *planar),
    __ASMPARM("d1", uint32_t planar_planemod),
    __ASMPARM("d2", uint32_t planar_wordmod));

#else
/* --- VBCC / old GCC: use __ASMPARM register declarations --- */

// 8 bitplane 320x256 c2p
void c2p1x1_8_c5_init(__ASMPARM("d0", int chunkyx),
                      __ASMPARM("d1", int chunkyy),
                      __ASMPARM("d2", int scroffsx),
                      __ASMPARM("d3", int scroffsy),
                      __ASMPARM("d4", int rowlen),
                      __ASMPARM("d5", int bplsize));

void c2p1x1_8_c5(__ASMPARM("a0", unsigned char *chunkyBuffer),
                 __ASMPARM("a1", unsigned char *planes));

// 5 bitplane no-modulo c2p
void c2p1x1_5_c5_060(__ASMPARM("a0", unsigned char *chunkyBuffer),
                     __ASMPARM("a1", unsigned char *planes),
                     __ASMPARM("d0", unsigned int pixels));

// 6 bitplane RGB c2p
void c2pRGB(__ASMPARM("a0", unsigned char *chunky),
            __ASMPARM("a1", unsigned char *bitplanes),
            __ASMPARM("d0", unsigned int w), __ASMPARM("d1", unsigned int h),
            __ASMPARM("d2", unsigned int stride_chunky),
            __ASMPARM("d3", unsigned int stride_bpls));

// 6 bitplane no-modulo c2p
void c2p1x1_6_c5_gen(__ASMPARM("a0", unsigned char *chunkyBuffer),
                     __ASMPARM("a1", unsigned char *planes),
                     __ASMPARM("d0", unsigned int pixels));

// 4 bitplane 320x180 c2p
void c2p1x1_4_c5_16_9_init(__ASMPARM("d0", int chunkyx),
                           __ASMPARM("d1", int chunkyy),
                           __ASMPARM("d2", int scroffsx),
                           __ASMPARM("d3", int scroffsy),
                           __ASMPARM("d4", int rowlen),
                           __ASMPARM("d5", int bplsize));

void c2p1x1_4_c5_16_9(__ASMPARM("a0", unsigned char *chunkyBuffer),
                      __ASMPARM("a1", unsigned char *planes));

// 4 bitplane 640x180 c2p
void c2p1x1_4_c5_16_9_h_init(__ASMPARM("d0", int chunkyx),
                             __ASMPARM("d1", int chunkyy),
                             __ASMPARM("d2", int scroffsx),
                             __ASMPARM("d3", int scroffsy),
                             __ASMPARM("d4", int rowlen),
                             __ASMPARM("d5", int bplsize));

void c2p1x1_4_c5_16_9_h(__ASMPARM("a0", unsigned char *chunkyBuffer),
                        __ASMPARM("a1", unsigned char *planes));

// 8 bitplane 320x256 screen size c2p
void c2p1x1_8_c5_040_init(__ASMPARM("d0", int chunkyx),
                          __ASMPARM("d1", int chunkyy),
                          __ASMPARM("d2", int scroffsx),
                          __ASMPARM("d3", int scroffsy),
                          __ASMPARM("d4", int rowlen),
                          __ASMPARM("d5", int bplsize),
                          __ASMPARM("d6", int chunkylen));

void c2p1x1_8_c5_040(__ASMPARM("a0", unsigned char *chunkyBuffer),
                     __ASMPARM("a1", unsigned char *planes));

// 8 bitplane 320x1 screen size c2p
void c2p1x1_8_c5_040_scanline_init(__ASMPARM("d0", int chunkyx),
                                   __ASMPARM("d1", int chunkyy),
                                   __ASMPARM("d2", int scroffsx),
                                   __ASMPARM("d3", int scroffsy),
                                   __ASMPARM("d4", int rowlen),
                                   __ASMPARM("d5", int bplsize),
                                   __ASMPARM("d6", int chunkylen));

void c2p1x1_8_c5_040_scanline(__ASMPARM("a0", unsigned char *chunkyBuffer),
                              __ASMPARM("a1", unsigned char *planes));

// 8 bitplane 320x180 screen size c2p
void c2p1x1_8_c5_040_16_9_init(__ASMPARM("d0", int chunkyx),
                               __ASMPARM("d1", int chunkyy),
                               __ASMPARM("d2", int scroffsx),
                               __ASMPARM("d3", int scroffsy),
                               __ASMPARM("d4", int rowlen),
                               __ASMPARM("d5", int bplsize),
                               __ASMPARM("d6", int chunkylen));

void c2p1x1_8_c5_040_16_9(__ASMPARM("a0", unsigned char *chunkyBuffer),
                          __ASMPARM("a1", unsigned char *planes));

// 6 bitplane arbitrary screen size c2p
void c2p1x1_6_c5_040_init(__ASMPARM("d0", int chunkyx),
                          __ASMPARM("d1", int chunkyy),
                          __ASMPARM("d2", int scroffsx),
                          __ASMPARM("d3", int scroffsy),
                          __ASMPARM("d4", int rowlen),
                          __ASMPARM("d5", int bplsize),
                          __ASMPARM("d6", int chunkylen));

void c2p1x1_6_c5_040(__ASMPARM("a0", unsigned char *chunkyBuffer),
                     __ASMPARM("a1", unsigned char *planes));

// 6 bitplane c2p
void c2p64(__ASMPARM("a0", unsigned char *chunky),
           __ASMPARM("a1", unsigned char *chunky_end),
           __ASMPARM("a2", unsigned char *bpls));

#ifdef __AMIGA__
// 1x1 8bpl cpu5 C2P for [almost] arbitrary BitMaps
void c2p1x1_8_c5_bm(__ASMPARM("a0", char *chunkybuffer),
                    __ASMPARM("a1", struct BitMap *bitmap),
                    __ASMPARM("d0", int chunkyxsize),
                    __ASMPARM("d1", int chunkyysize),
                    __ASMPARM("d2", int xoffset), __ASMPARM("d3", int yoffset));
#endif

// 1x1 8bpl cpu5, one scanline.
void c2p_8bpl_scanline(
    __ASMPARM("d0", int pixels), __ASMPARM("a0", uint32_t *chunky),
    __ASMPARM("a1", uint32_t *planar),
    __ASMPARM("d1", uint32_t planar_planemod), // bytes between each plane
    __ASMPARM("d2", uint32_t planar_wordmod) // bytes between each 32pixel word
);

#endif /* __GCC_ELF__ */

#endif
