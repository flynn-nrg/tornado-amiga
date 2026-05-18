#ifndef LZW_UNPACK_INNER_H
#define LZW_UNPACK_INNER_H

#include "asmparm.h"

#ifdef __GCC_ELF__

static inline void lzw_unpack_inner_12(uint8_t *compressed, uint8_t *dest,
                                       uint8_t **symbols, uint8_t *lengths,
                                       uint32_t clear, uint32_t stop) {
  register uint8_t *_a0 __asm__("a0") = compressed;
  register uint8_t *_a1 __asm__("a1") = dest;
  register uint8_t **_a2 __asm__("a2") = symbols;
  register uint8_t *_a3 __asm__("a3") = lengths;
  register uint32_t _d2 __asm__("d2") = clear;
  register uint32_t _d3 __asm__("d3") = stop;
  __asm__ volatile("jsr _lzw_unpack_inner_12"
                   : "+r"(_a0), "+r"(_a1), "+r"(_a2), "+r"(_a3), "+r"(_d2),
                     "+r"(_d3)
                   :
                   : "cc", "memory");
}

static inline void lzw_unpack_inner_16(uint8_t *compressed, uint8_t *dest,
                                       uint8_t **symbols, uint8_t *lengths,
                                       uint32_t clear, uint32_t stop) {
  register uint8_t *_a0 __asm__("a0") = compressed;
  register uint8_t *_a1 __asm__("a1") = dest;
  register uint8_t **_a2 __asm__("a2") = symbols;
  register uint8_t *_a3 __asm__("a3") = lengths;
  register uint32_t _d2 __asm__("d2") = clear;
  register uint32_t _d3 __asm__("d3") = stop;
  __asm__ volatile("jsr _lzw_unpack_inner_16"
                   : "+r"(_a0), "+r"(_a1), "+r"(_a2), "+r"(_a3), "+r"(_d2),
                     "+r"(_d3)
                   :
                   : "cc", "memory");
}

#else

void lzw_unpack_inner_12(__ASMPARM("a0", uint8_t *compressed),
                         __ASMPARM("a1", uint8_t *dest),
                         __ASMPARM("a2", uint8_t **symbols),
                         __ASMPARM("a3", uint8_t *lengths),
                         __ASMPARM("d2", uint32_t clear),
                         __ASMPARM("d3", uint32_t stop));

void lzw_unpack_inner_16(__ASMPARM("a0", uint8_t *compressed),
                         __ASMPARM("a1", uint8_t *dest),
                         __ASMPARM("a2", uint8_t **symbols),
                         __ASMPARM("a3", uint8_t *lengths),
                         __ASMPARM("d2", uint32_t clear),
                         __ASMPARM("d3", uint32_t stop));

#endif

#endif
