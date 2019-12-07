
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "c2p.h"

#define c2p_merge_one(x, y, t, s, m) \
    t    = y; \
    t  >>= s; \
    t   ^= x; \
    t   &= m; \
    x   ^= t; \
    t  <<= s; \
    y   ^= t;

#define c2p_merge_one_16(x,y,t) c2p_merge_one(x, y, t, 16, 0x0000ffff)
#define c2p_merge_one_8(x,y,t)  c2p_merge_one(x, y, t,  8, 0x00ff00ff)
#define c2p_merge_one_4(x,y,t)  c2p_merge_one(x, y, t,  4, 0x0f0f0f0f)
#define c2p_merge_one_2(x,y,t)  c2p_merge_one(x, y, t,  2, 0x33333333)
#define c2p_merge_one_1(x,y,t)  c2p_merge_one(x, y, t,  1, 0x55555555)

void c2p_8bpl_scanline(
    __ASMPARM("d0", int pixels),
    __ASMPARM("a0", uint32_t *chunky),
    __ASMPARM("a1", uint32_t *planar),
    __ASMPARM("d1", uint32_t planar_planemod),  // bytes between each plane
    __ASMPARM("d2", uint32_t planar_wordmod)    // bytes between each 32pixel word
) {
    assert((pixels % 32) == 0);
    assert((planar_planemod % 4) == 0);
    assert((planar_wordmod  % 4) == 0);

    uint32_t *chunky_end = chunky + pixels / 4;
    planar_planemod /= 4;
    planar_wordmod  /= 4;

    uint32_t d0,d1,d2,d3,d4,d5,d6,d7;
    uint32_t tmp1;

    while (chunky < chunky_end) {
        d0 = *chunky++; d1 = *chunky++; d2 = *chunky++; d3 = *chunky++;
        d4 = *chunky++; d5 = *chunky++; d6 = *chunky++; d7 = *chunky++;

        c2p_merge_one_4(d0, d1, tmp1);
        c2p_merge_one_4(d2, d3, tmp1);
        c2p_merge_one_4(d4, d5, tmp1);
        c2p_merge_one_4(d6, d7, tmp1);

        c2p_merge_one_16(d0, d4, tmp1);
        c2p_merge_one_16(d1, d5, tmp1);
        c2p_merge_one_16(d2, d6, tmp1);
        c2p_merge_one_16(d3, d7, tmp1);

        c2p_merge_one_2(d0, d4, tmp1);
        c2p_merge_one_2(d1, d5, tmp1);
        c2p_merge_one_2(d2, d6, tmp1);
        c2p_merge_one_2(d3, d7, tmp1);

        c2p_merge_one_8(d0, d2, tmp1);
        c2p_merge_one_8(d1, d3, tmp1);
        c2p_merge_one_8(d4, d6, tmp1);
        c2p_merge_one_8(d5, d7, tmp1);

        c2p_merge_one_1(d0, d2, tmp1);
        c2p_merge_one_1(d1, d3, tmp1);
        c2p_merge_one_1(d4, d6, tmp1);
        c2p_merge_one_1(d5, d7, tmp1);

        planar[0 * planar_planemod] = d7;
        planar[1 * planar_planemod] = d5;
        planar[2 * planar_planemod] = d3;
        planar[3 * planar_planemod] = d1;
        planar[4 * planar_planemod] = d6;
        planar[5 * planar_planemod] = d4;
        planar[6 * planar_planemod] = d2;
        planar[7 * planar_planemod] = d0;
        planar += planar_wordmod;
    }
}

