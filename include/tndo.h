/*
Copyright (c) 2019 Miguel Mendez

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software in a
product, an acknowledgment in the product documentation would be appreciated but
is not required.

    2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#ifndef TNDO_H
#define TNDO_H

#define TNDO_MAGIC "TNDO"
#define TNDO_MAGIC_INT 0x544e444f

#define TNDO_TYPE_GENERIC 1
#define TNDO_TYPE_GFX 2
#define TNDO_TYPE_AUDIO 3

#define TNDO_COMPRESSION_NONE 1
#define TNDO_COMPRESSION_DLTA 2
#define TNDO_COMPRESSION_ZLIB 3
#define TNDO_COMPRESSION_DDPCM 4
#define TNDO_COMPRESSION_DLTA_14 5
#define TNDO_COMPRESSION_LZH 8
#define TNDO_COMPRESSION_LZSS 9
#define TNDO_COMPRESSION_LZW 10

// This is not actually a compression setting but used
// in tndo_compress to find the best suited algorithm.
#define TNDO_COMPRESSION_BEST 999

#define TNDO_GFX_MODE_CHUNKY 1
#define TNDO_GFX_MODE_PLANAR 2
#define TNDO_GFX_MODE_SPRITE 3
#define TNDO_GFX_MODE_COPPERLIST 4
#define TNDO_GFX_MODE_PALETTE 5

// The TNDO header is stored on disk in Network order!!!
typedef struct {
  uint32_t magic;
  uint32_t type;
  uint32_t compression;
  uint32_t compressed_size;
  uint32_t uncompressed_size;
  // For graphics data.
  uint32_t gfx_sizex;
  uint32_t gfx_sizey;
  uint32_t gfx_depth;
  uint32_t gfx_mode;
  // For audio data.
  uint32_t sampleRate;
  uint32_t maxBits;
  uint32_t segmentSize;
  uint32_t numSegments;
  // Audio encoding metadata.
  uint32_t numSamples;
  uint32_t numChannels;
  uint32_t bitsPerSample;
  uint32_t addBits;
  uint32_t capsModel;
} TndoHeader;

TndoHeader *openTNDO(FILE *);

#endif
