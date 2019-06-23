#ifndef INCLUDE_ZOOM_INNER_H
#define INCLUDE_ZOOM_INNER_H

#include "asmparm.h"

void renderZoom_asm(__ASMPARM("a0", unsigned char ***allTxtPtr),
                    __ASMPARM("a1", unsigned char *chunky),
                    __ASMPARM("a2", zoomIter *iteration));

#endif
