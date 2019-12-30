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

#include "assets.h"
#include "ddpcm.h"
#include "ddpcm_decode.h"
#include "ddpcm_loader.h"
#include "ddpcm_lowlevel.h"
#include "lzh_loader.h"
#include "lzss_loader.h"
#include "lzw_loader.h"
#include "memory.h"
#include "paula_output.h"
#include "ptr_bridges.h"
#include "tndo.h"
#include "tndo_assert.h"
#include "tndo_file.h"
#include "tornado_settings.h"

static const char *fileList[256];
static int numFiles = 0;

static ddpcmHeader *ddpcmData;
static ddpcmDecodedData *decodedData;

void emit_container_script(const char *fileName, const char *containerName) {
  FILE *fd = fopen(fileName, "w");
  if (!fd) {
    fprintf(stderr, "FATAL - Cannot open <%s> for writing.\n", fileName);
    return;
  }
  fprintf(fd, "#!/bin/sh\n");
  fprintf(fd, "/tmp/assemble_assets -o %s ", containerName);
  for (int i = 0; i < numFiles; i++) {
    fprintf(fd, " %s ", fileList[i]);
  }
  fprintf(fd, "\n");
  fclose(fd);
}

int loadAssets(void **demoAssets, const char *const *assetList, int *assetSizes,
               int numAssets, int tornadoOptions, demoParams *dp) {
  int num_errors = 0;
  int i;
  int res;
  int read;
  long size;
  FILE *fd;

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("\nDEBUG - Loading and unpacking data");
  }

  int allocFlag = 0;
  if (tornadoOptions & ASSETS_IN_REUSABLE_MEM) {
    allocFlag |= TNDO_REUSABLE_MEM;
  }
  for (i = 0; i < numAssets; i++) {

    if (tornadoOptions & EMIT_CONTAINER_SCRIPT) {
      tndo_assert(numFiles < 256);
      fileList[numFiles] = (char *)assetList[i];
      numFiles++;
    }

    if (tornadoOptions & VERBOSE_DEBUGGING) {
      printf(".");
    }

    fd = tndo_fopen(assetList[i], "r");
    if (!fd) {
      printf("\nMissing asset: %s\n", assetList[i]);
      num_errors++;
      continue;
    }

    // Check for TNDO signature
    TndoHeader *th = openTNDO(fd);
    if (!th) {
      tndo_fseek(fd, 0, SEEK_END);
      size = tndo_ftell(fd);
      tndo_fseek(fd, 0, SEEK_SET);

      demoAssets[i] = tndo_malloc(size, allocFlag);
      assetSizes[i] = size;
      if (!demoAssets[i]) {
        printf("\nCant allocate memory for asset: %s\n", assetList[i]);
        num_errors++;
        continue;
      }
      read = tndo_fread(demoAssets[i], size, 1, fd);
      if (!read)
        return 0;

      tndo_fclose(fd);
    } else {
      switch (ENDI4(th->type)) {
      case TNDO_TYPE_AUDIO:
        if (tornadoOptions & VERBOSE_DEBUGGING) {
          printf("DEBUG - Loading Audio asset...\n");
        }
        if (tornadoOptions & VERBOSE_DEBUGGING) {
          printf("DEBUG - Sample rate: %u\n", ENDI4(th->sampleRate));
        }
        switch (ENDI4(th->sampleRate)) {
        case 11025:
          dp->audioPeriod = REPLAY_PERIOD_11025;
          break;
        case 22050:
          dp->audioPeriod = REPLAY_PERIOD_22050;
          break;
        default:
          printf("FATAL - Unsupported sample rate. Aborting.\n");
          abort();
        }

        if (tornadoOptions & VERBOSE_DEBUGGING) {
          printf("DEBUG - Bits per sample: %u\n", ENDI4(th->bitsPerSample));
        }
        switch (ENDI4(th->bitsPerSample)) {
        case 8:
          dp->audioMode = OUTPUT_8_BIT_STEREO;
          break;
        case 16:
          dp->audioMode = OUTPUT_14_BIT_STEREO;
          break;
        default:
          printf("FATAL - Unsupported bits per sample setting. Aborting.\n");
          abort();
        }

        switch (ENDI4(th->compression)) {
        case TNDO_COMPRESSION_DDPCM:
          if (tornadoOptions & VERBOSE_DEBUGGING) {
            printf("DEBUG - Using DDPCM encoding.\n");
          }
          ddpcmData = ddpcmLoadFile(fd, tornadoOptions);
          if (!ddpcmData) {
            printf("FATAL - Loading audio assets failed. Aborting.\n");
            abort();
          }

          initDDPCM_Decoder(ddpcmData->qtablesLeft, ddpcmData->qtablesRight,
                            ddpcmData->scalesLeft, ddpcmData->scalesRight,
                            ddpcmData->left, ddpcmData->right,
                            ddpcmData->numFrames, ddpcmData->framesPerQTable);

          dp->tornadoOptions |= DDPCM_STREAMING;
          dp->sampleRate = ENDI4(th->sampleRate);
          dp->bitsPerSample = ENDI4(th->bitsPerSample);
          // Dummy mix states to make the Paula output routines happy.
          dp->mixState = (char **)&decodedData;
          dp->mixState2 = (char **)&decodedData;
          demoAssets[i] = (unsigned int *)0xdeadbeef;
          assetSizes[i] = 4;
          break;
        default:
          printf("FATAL - Unsupported compression setting. Aborting.\n");
          abort();
        }

        break;

      case TNDO_TYPE_GFX:
        // TODO(flynn): Implement this
        break;

      case TNDO_TYPE_GENERIC:
        switch (ENDI4(th->compression)) {

        case TNDO_COMPRESSION_NONE:
          assetSizes[i] = ENDI4(th->uncompressed_size);
          demoAssets[i] = tndo_malloc(ENDI4(th->uncompressed_size), allocFlag);
          if (!demoAssets[i]) {
            printf("\nCant allocate memory for asset: %s\n", assetList[i]);
            num_errors++;
          }
          read = tndo_fread(demoAssets[i], ENDI4(th->uncompressed_size), 1, fd);
          if (!read)
            return 0;
          break;

        case TNDO_COMPRESSION_LZW:
          if (tornadoOptions & NO_Z_DECOMPRESS) {
            assetSizes[i] = ENDI4(th->uncompressed_size);
            demoAssets[i] = tndo_malloc(ENDI4(th->compressed_size), allocFlag);
            if (!demoAssets[i]) {
              printf("\nCant allocate memory for asset: %s\n", assetList[i]);
              num_errors++;
              break;
            }

            read = tndo_fread(demoAssets[i], ENDI4(th->compressed_size), 1, fd);
            if (!read)
              return 0;
          } else {
            assetSizes[i] = ENDI4(th->uncompressed_size);
            demoAssets[i] =
                tndo_malloc(ENDI4(th->uncompressed_size), allocFlag);
            if (!demoAssets[i]) {
              printf("\nCant allocate memory for asset: %s\n", assetList[i]);
              num_errors++;
              break;
            }
            res = lzwLoadFile(fd, demoAssets[i], ENDI4(th->compressed_size));
            if (res != 0) {
              fprintf(stderr, "WARNING - Error unpacking file %s. Skipping.\n",
                      assetList[i]);
              return 0;
            }
          }
          break;

        case TNDO_COMPRESSION_LZH:
          if (tornadoOptions & NO_Z_DECOMPRESS) {
            assetSizes[i] = ENDI4(th->uncompressed_size);
            demoAssets[i] = tndo_malloc(ENDI4(th->compressed_size), allocFlag);
            if (!demoAssets[i]) {
              printf("\nCant allocate memory for asset: %s\n", assetList[i]);
              num_errors++;
              break;
            }

            read = tndo_fread(demoAssets[i], ENDI4(th->compressed_size), 1, fd);
            if (!read)
              return 0;
          } else {
            assetSizes[i] = ENDI4(th->uncompressed_size);
            demoAssets[i] =
                tndo_malloc(ENDI4(th->uncompressed_size), allocFlag);
            if (!demoAssets[i]) {
              printf("\nCant allocate memory for asset: %s\n", assetList[i]);
              num_errors++;
              break;
            }
            res = lzhLoadFile(fd, demoAssets[i], ENDI4(th->compressed_size));
            if (res != 0) {
              fprintf(stderr, "WARNING - Error unpacking file %s. Skipping.\n",
                      assetList[i]);
              return 0;
            }
          }
          break;

        case TNDO_COMPRESSION_LZSS:
          if (tornadoOptions & NO_Z_DECOMPRESS) {
            assetSizes[i] = ENDI4(th->uncompressed_size);
            demoAssets[i] = tndo_malloc(ENDI4(th->compressed_size), allocFlag);
            if (!demoAssets[i]) {
              printf("\nCant allocate memory for asset: %s\n", assetList[i]);
              num_errors++;
              break;
            }

            read = tndo_fread(demoAssets[i], ENDI4(th->compressed_size), 1, fd);
            if (!read)
              return 0;
          } else {
            assetSizes[i] = ENDI4(th->uncompressed_size);
            demoAssets[i] =
                tndo_malloc(ENDI4(th->uncompressed_size), allocFlag);
            if (!demoAssets[i]) {
              printf("\nCant allocate memory for asset: %s\n", assetList[i]);
              num_errors++;
              break;
            }
            res = lzssLoadFile(fd, demoAssets[i], ENDI4(th->compressed_size));
            if (res != 0) {
              fprintf(stderr, "WARNING - Error unpacking file %s. Skipping.\n",
                      assetList[i]);
              return 0;
            }
          }
          break;

        default:
          fprintf(stderr,
                  "WARNING - Unsupported compression setting %x for file %s. "
                  "Skipping.\n",
                  ENDI4(th->compression), assetList[i]);
          break;
        }
      }

      tndo_fclose(fd);
    }
  }

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("\n");
  }

  if (num_errors > 0) {
    return 0;
  }

  return 1;
}
