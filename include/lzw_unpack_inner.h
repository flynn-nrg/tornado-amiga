#ifndef LZW_UNPACK_INNER_H
#define LZW_UNPACK_INNER_H

#include "asmparm.h"

void lzw_unpack_inner_12(
    __ASMPARM("a0", uint8_t *compressed),
    __ASMPARM("a1", uint8_t *dest),
    __ASMPARM("a2", uint8_t **symbols),
    __ASMPARM("a3", uint8_t *lengths),
    __ASMPARM("d2", uint32_t clear),
    __ASMPARM("d3", uint32_t stop)
);

void lzw_unpack_inner_16(
    __ASMPARM("a0", uint8_t *compressed),
    __ASMPARM("a1", uint8_t *dest),
    __ASMPARM("a2", uint8_t **symbols),
    __ASMPARM("a3", uint8_t *lengths),
    __ASMPARM("d2", uint32_t clear),
    __ASMPARM("d3", uint32_t stop)
);


#endif
