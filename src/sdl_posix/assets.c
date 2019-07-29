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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "assets.h"
#include "caps_loader.h"
#include "ddpcm.h"
#include "ddpcm_decode.h"
#include "ddpcm_loader.h"
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

static capsData_t *audioData;
static ddpcmHeader *ddpcmData;
static ddpcmDecodedData *decodedData;

static const char *fileList[256];
static int numFiles = 0;

// TODO(flynn): Do this for real.
static void installAssetWatcher(void **asset, char *assetName, int *assetSize,
                                int tornadoOptions, demoParams *dp) {
  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("\nDEBUG - Installed asset watcher for: %s\n", assetName);
  }
}

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

static int loadAsset(void **asset, char *assetName, int *assetSize,
                     int tornadoOptions, demoParams *dp) {
  FILE *fd;
  int size;
  int read;
  int res;
  int allocFlag = 0;

  if (tornadoOptions & ASSETS_IN_REUSABLE_MEM) {
    allocFlag |= TNDO_REUSABLE_MEM;
  }

  if (tornadoOptions & EMIT_CONTAINER_SCRIPT) {
    tndo_assert(numFiles < 256);
    fileList[numFiles] = assetName;
    numFiles++;
  }

  fd = tndo_fopen(assetName, "r");
  if (!fd) {
    printf("\nMissing asset: %s\n", assetName);
    return 0;
  }

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("\nDEBUG - Loading asset: %s\n", assetName);
  }

  // Check for TNDO signature
  TndoHeader *th = openTNDO(fd);
  if (!th) {
    tndo_fseek(fd, 0, SEEK_END);
    size = tndo_ftell(fd);
    tndo_fseek(fd, 0, SEEK_SET);

    *asset = tndo_malloc(size, allocFlag);
    *assetSize = size;
    if (!*asset) {
      printf("\nCant allocate memory for asset: %s\n", assetName);
      return 0;
    }
    read = tndo_fread(*asset, size, 1, fd);
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
        printf("DEBUG - Sample rate: %i\n", ENDI4(th->sampleRate));
      }
      switch (ENDI4(th->sampleRate)) {
      case 11025:
        dp->audioPeriod = REPLAY_PERIOD_11025;
        break;
      case 22050:
        dp->audioPeriod = REPLAY_PERIOD_22050;
        break;
      case 28150:
        dp->audioPeriod = REPLAY_PERIOD_28150;
        break;
      case 28867:
        dp->audioPeriod = REPLAY_PERIOD_28867;
        break;
      default:
        printf("FATAL - Unsupported sample rate. Aborting.\n");
        abort();
      }

      if (tornadoOptions & VERBOSE_DEBUGGING) {
        printf("DEBUG - Bits per sample: %i\n", ENDI4(th->bitsPerSample));
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
      case TNDO_COMPRESSION_CAPS:
        if (tornadoOptions & VERBOSE_DEBUGGING) {
          printf("DEBUG - Using CAPS encoding.\n");
        }
        audioData =
            loadCaps(fd, ENDI4(th->numSamples), ENDI4(th->bitsPerSample),
                     ENDI4(th->addBits), ENDI4(th->capsModel), tornadoOptions);
        if (!audioData) {
          printf("FATAL - Loading audio assets failed. Aborting.\n");
          abort();
        }
        dp->sampleRate = ENDI4(th->sampleRate);
        dp->bitsPerSample = ENDI4(th->bitsPerSample);
        dp->mixState = &audioData->left_buffer;
        dp->mixState2 = &audioData->right_buffer;
        *asset = (unsigned int *)0xdeadbeef;
        *assetSize = 4;
        break;
      case TNDO_COMPRESSION_DDPCM:
        if (tornadoOptions & VERBOSE_DEBUGGING) {
          printf("DEBUG - Using DDPCM encoding.\n");
        }
        ddpcmData = ddpcmLoadFile(fd, tornadoOptions);
        if (!ddpcmData) {
          printf("FATAL - Loading audio assets failed. Aborting.\n");
          abort();
        }

        decodedData = decodeDDPCMStream(ddpcmData);

        dp->sampleRate = ENDI4(th->sampleRate);
        dp->bitsPerSample = ENDI4(th->bitsPerSample);
        dp->mixState = (char **)&decodedData->left;
        dp->mixState2 = (char **)&decodedData->right;
        *asset = (unsigned int *)0xdeadbeef;
        *assetSize = 4;
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
        *assetSize = ENDI4(th->uncompressed_size);
        *asset = tndo_malloc(ENDI4(th->uncompressed_size), allocFlag);
        if (!*asset) {
          printf("\nCant allocate memory for asset: %s\n", assetName);
        }
        read = tndo_fread(*asset, ENDI4(th->uncompressed_size), 1, fd);
        if (!read)
          return 0;
        break;

      case TNDO_COMPRESSION_LZH:
        if (tornadoOptions & NO_Z_DECOMPRESS) {
          *assetSize = ENDI4(th->uncompressed_size);
          *asset = tndo_malloc(ENDI4(th->compressed_size), allocFlag);
          if (!*asset) {
            printf("\nCant allocate memory for asset: %s\n", assetName);
            break;
          }

          read = tndo_fread(*asset, ENDI4(th->compressed_size), 1, fd);
          if (!read)
            return 0;
        } else {
          *assetSize = ENDI4(th->uncompressed_size);
          *asset = tndo_malloc(ENDI4(th->uncompressed_size), allocFlag);
          if (!*asset) {
            printf("\nCant allocate memory for asset: %s\n", assetName);
            break;
          }
          res = lzhLoadFile(fd, (unsigned char *)*asset,
                            ENDI4(th->compressed_size));
          if (res != 0) {
            fprintf(stderr, "WARNING - Error unpacking file %s. Skipping.\n",
                    assetName);
            return 0;
          }
        }
        break;

      case TNDO_COMPRESSION_LZW:
        if (tornadoOptions & NO_Z_DECOMPRESS) {
          *assetSize = ENDI4(th->uncompressed_size);
          *asset = tndo_malloc(ENDI4(th->compressed_size), allocFlag);
          if (!*asset) {
            printf("\nCant allocate memory for asset: %s\n", assetName);
            break;
          }

          read = tndo_fread(*asset, ENDI4(th->compressed_size), 1, fd);
          if (!read)
            return 0;
        } else {
          *assetSize = ENDI4(th->uncompressed_size);
          *asset = tndo_malloc(ENDI4(th->uncompressed_size), allocFlag);
          if (!*asset) {
            printf("\nCant allocate memory for asset: %s\n", assetName);
            break;
          }
          res = lzwLoadFile(fd, (unsigned char *)*asset,
                            ENDI4(th->compressed_size));
          if (res != 0) {
            fprintf(stderr, "WARNING - Error unpacking file %s. Skipping.\n",
                    assetName);
            return 0;
          }
        }
        break;

      case TNDO_COMPRESSION_LZSS:
        if (tornadoOptions & NO_Z_DECOMPRESS) {
          *assetSize = ENDI4(th->uncompressed_size);
          *asset = tndo_malloc(ENDI4(th->compressed_size), allocFlag);
          if (!*asset) {
            printf("\nCant allocate memory for asset: %s\n", assetName);
            break;
          }

          read = tndo_fread(*asset, ENDI4(th->compressed_size), 1, fd);
          if (!read)
            return 0;
        } else {
          *assetSize = ENDI4(th->uncompressed_size);
          *asset = tndo_malloc(ENDI4(th->uncompressed_size), allocFlag);
          if (!*asset) {
            printf("\nCant allocate memory for asset: %s\n", assetName);
            break;
          }
          res = lzssLoadFile(fd, (unsigned char *)*asset,
                             ENDI4(th->compressed_size));
          if (res != 0) {
            fprintf(stderr, "WARNING - Error unpacking file %s. Skipping.\n",
                    assetName);
            return 0;
          }
        }
        break;

      default:
        fprintf(stderr,
                "WARNING - Unsupported compression setting %x for file %s. "
                "Skipping.\n",
                ENDI4(th->compression), assetName);
        break;
      }
    }

    tndo_fclose(fd);
  }

  return 1;
}

int loadAssets(void **demoAssets, const char *const *assetList, int *assetSizes,
               int numAssets, int tornadoOptions, demoParams *dp) {

  int i;
  int res;
  int num_errors = 0;

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("\nDEBUG - Loading and unpacking data");
  }

  for (i = 0; i < numAssets; i++) {

    if (tornadoOptions & HOTSWAPPABLE_ASSETS) {
      installAssetWatcher((void **)demoAssets[i], (char *)assetList[i],
                          &assetSizes[i], tornadoOptions, dp);
    }

    res = loadAsset(&demoAssets[i], (char *)assetList[i], &assetSizes[i],
                    tornadoOptions, dp);
    if (!res)
      num_errors++;
  }

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("\n");
  }

  if (num_errors > 0) {
    return 0;
  }

  return 1;
}
