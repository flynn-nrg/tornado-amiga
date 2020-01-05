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
