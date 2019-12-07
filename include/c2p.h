#ifndef INCLUDE_C2P_H
#define INCLUDE_C2P_H

#include "asmparm.h"

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
    __ASMPARM("d0", int pixels),
    __ASMPARM("a0", uint32_t *chunky),
    __ASMPARM("a1", uint32_t *planar),
    __ASMPARM("d1", uint32_t planar_planemod),  // bytes between each plane
    __ASMPARM("d2", uint32_t planar_wordmod)    // bytes between each 32pixel word
);

#endif
