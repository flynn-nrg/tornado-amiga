#ifndef DEPACKER_DOYNAX_H
#define DEPACKER_DOYNAX_H

#include "asmparm.h"

void doynaxdepack(
    __ASMPARM("a0", unsigned char *src),
    __ASMPARM("a1", unsigned char *dst)
);

#endif
