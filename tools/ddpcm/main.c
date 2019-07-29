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

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tndo.h>

#include "ddpcm.h"
#include "quantiser.h"
#include "wav.h"

static void *loadFile(FILE *infile) {
  int size;
  int read;

  fseek(infile, 0, SEEK_END);
  size = ftell(infile);
  fseek(infile, 0, SEEK_SET);

  void *data = malloc(size);
  if (!data)
    return 0;

  read = fread(data, size, 1, infile);
  if (!read)
    return 0;

  return data;
}

static void usage(char *progname) {

  fprintf(stderr,
          "Usage: %s -q -d <delta-computation> -i <input_file> -o "
          "<output_file> -p <preview_file>\n",
          progname);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {

  int ch;
  uint32_t quiet = 0;
  uint32_t save_preview = 0;
  uint32_t deltaMode = DELTA_MODE_TOP;
  FILE *infile, *outfile, *previewfile;

  if (argc < 4) {
    usage(argv[0]);
  }

  while ((ch = getopt(argc, argv, "d:qi:o:p:")) != -1) {
    switch (ch) {
    case 'i':
      infile = fopen(optarg, "r");
      if (!infile) {
        fprintf(stderr, "FATAL - Cannot open <%s> for reading.\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;
    case 'o':
      outfile = fopen(optarg, "w");
      if (!outfile) {
        fprintf(stderr, "FATAL - Cannot open <%s> for writing.\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;
    case 'p':
      previewfile = fopen(optarg, "w");
      if (!previewfile) {
        fprintf(stderr, "FATAL - Cannot open <%s> for writing.\n", optarg);
        exit(EXIT_FAILURE);
      }
      save_preview = 1;
      break;
    case 'q':
      quiet = 1;
      break;
    case 'd':
      deltaMode = atoi(optarg);
      break;
    case '?':
    default:
      usage(argv[0]);
    }
  }
  argc -= optind;
  argv += optind;

  void *data = loadFile(infile);
  fclose(infile);
  WavHeader *wh = parseWavHeader(data);

  if (!wh) {
    exit(EXIT_FAILURE);
  }

  if (!quiet) {
    printf("\n");
    printf("Tornado DDPCM encoder. (C) 2019 Miguel Mendez.\n");
    printf("\n");
    printf("Audio data information:\n");
    printf("-----------------------\n");
    printf("Bits per sample: %i\n", wh->bitsPerSample);
    printf("Sample rate: %iHz\n", wh->sampleRate);
    printf("Size: %i\n", wh->dataLen);
    printf("\n");
  }

  AudioData *ad = splitChannels(wh);

  int32_t numFrames = ad->numSamples / DDPCM_FRAME_NUMSAMPLES;
  uint32_t fpqt =
      framesPerQtable(numFrames, DDPCM_MAX_TABLES, DDPCM_MIN_TABLES);

  if (!quiet) {
    printf("Per channel data:\n");
    printf("-----------------\n");
    printf("Number of samples: %i\n", ad->numSamples);
    printf("Sample size: %i\n", ad->sampleSize);
    printf("Frame size: %u samples\n", DDPCM_FRAME_NUMSAMPLES);
    printf("Number of frames: %u\n", numFrames);
    printf("Frames per quantisation table: %u\n", fpqt);
    printf("\n");
  }

  uint32_t samplesPerTable = fpqt * DDPCM_FRAME_NUMSAMPLES;

  ddpcmHeader *ddpcmh = calloc(1, sizeof(ddpcmHeader));
  ddpcmh->numSamples = ad->numSamples;
  ddpcmh->numFrames = numFrames;
  ddpcmh->numQTables = numFrames / fpqt;
  ddpcmh->framesPerQTable = fpqt;
  ddpcmh->qtablesLeft = calloc(ddpcmh->numQTables, sizeof(q_entry *));
  ddpcmh->qtablesRight = calloc(ddpcmh->numQTables, sizeof(q_entry *));
  ddpcmh->scalesLeft = calloc(ddpcmh->numFrames, sizeof(uint8_t));
  ddpcmh->scalesRight = calloc(ddpcmh->numFrames, sizeof(uint8_t));
  ddpcmh->left = calloc(ddpcmh->numFrames, DDPCM_COMPRESSED_FRAME_SIZE);
  ddpcmh->right = calloc(ddpcmh->numFrames, DDPCM_COMPRESSED_FRAME_SIZE);

  // Left channel...
  int16_t *leftSrc = (int16_t *)ad->left;
  uint32_t lOffset = 0;
  uint32_t lDone = 0;
  uint8_t *lDest = ddpcmh->left;
  uint32_t lDstOffset = 0;
  uint32_t lFrame = 0;
  for (uint32_t i = 0; i < ddpcmh->numQTables; i++) {
    ddpcmh->qtablesLeft[i] =
        quantiser(&leftSrc[lOffset], samplesPerTable, deltaMode);
    for (uint32_t j = 0; j < ddpcmh->framesPerQTable; j++) {
      ddpcmh->scalesLeft[lFrame++] = encodeFrame(
          &leftSrc[lOffset], &lDest[lDstOffset], ddpcmh->qtablesLeft[i]);
      lOffset += DDPCM_FRAME_NUMSAMPLES;
      lDstOffset += DDPCM_COMPRESSED_FRAME_SIZE;
      printf("Compressing left channel: %i/%i\r", lDone++, ddpcmh->numFrames);
    }
  }

  printf("\n");

  // Right channel...
  int16_t *rightSrc = (int16_t *)ad->right;
  uint32_t rOffset = 0;
  uint32_t rDone = 0;
  uint8_t *rDest = ddpcmh->right;
  uint32_t rDstOffset = 0;
  uint32_t rFrame = 0;
  for (uint32_t i = 0; i < ddpcmh->numQTables; i++) {
    ddpcmh->qtablesRight[i] =
        quantiser(&rightSrc[rOffset], samplesPerTable, deltaMode);
    for (uint32_t j = 0; j < ddpcmh->framesPerQTable; j++) {
      ddpcmh->scalesRight[rFrame++] = encodeFrame(
          &rightSrc[rOffset], &rDest[rDstOffset], ddpcmh->qtablesRight[i]);
      rOffset += DDPCM_FRAME_NUMSAMPLES;
      rDstOffset += DDPCM_COMPRESSED_FRAME_SIZE;
      printf("Compressing right channel: %i/%i\r", rDone++, ddpcmh->numFrames);
    }
  }

  printf("\n");
  printf("\n");

  uint32_t qtableUsage = ddpcmh->numQTables * 2 * 64 * 2;
  uint32_t framesUsage = ddpcmh->numFrames * 2 * DDPCM_COMPRESSED_FRAME_SIZE;
  uint32_t scalesUsage = ddpcmh->numFrames * 2;
  printf("Uncompressed size: %u bytes\n", wh->dataLen);
  printf("Compressed size: %u bytes (%u bytes in qtables, %u bytes in scale "
         "tables, %u bytes in compressed frames)\n",
         qtableUsage + framesUsage + scalesUsage, qtableUsage, scalesUsage,
         framesUsage);

  if (save_preview) {
    // Unpack
    // Left channel...
    uint8_t *uleftSrc = ddpcmh->left;
    uint32_t ulOffset = 0;
    uint32_t ulDone = 0;
    int16_t *ulDest = (int16_t *)ad->left;
    uint32_t ulDstOffset = 0;
    uint32_t ulFrame = 0;
    for (uint32_t i = 0; i < ddpcmh->numQTables; i++) {
      for (uint32_t j = 0; j < ddpcmh->framesPerQTable; j++) {
        decodeFrame(&uleftSrc[ulOffset], &ulDest[ulDstOffset],
                    ddpcmh->qtablesLeft[i], ddpcmh->scalesLeft[ulFrame++]);
        ulOffset += DDPCM_COMPRESSED_FRAME_SIZE;
        ulDstOffset += DDPCM_FRAME_NUMSAMPLES;
        printf("Decompressing left channel: %i/%i\r", ulDone++,
               ddpcmh->numFrames);
      }
    }

    printf("\n");

    // Right channel...
    uint8_t *urightSrc = ddpcmh->right;
    uint32_t urOffset = 0;
    uint32_t urDone = 0;
    int16_t *urDest = (int16_t *)ad->right;
    uint32_t urDstOffset = 0;
    uint32_t urFrame = 0;
    for (uint32_t i = 0; i < ddpcmh->numQTables; i++) {
      for (uint32_t j = 0; j < ddpcmh->framesPerQTable; j++) {
        decodeFrame(&urightSrc[urOffset], &urDest[urDstOffset],
                    ddpcmh->qtablesRight[i], ddpcmh->scalesRight[urFrame++]);
        urOffset += DDPCM_COMPRESSED_FRAME_SIZE;
        urDstOffset += DDPCM_FRAME_NUMSAMPLES;
        printf("Decompressing right channel: %i/%i\r", urDone++,
               ddpcmh->numFrames);
      }
    }

    printf("\n");

    WavHeader *wout = joinChannels(ad);
    writeWAV(previewfile, wout);
    fclose(previewfile);
  }

  // TNDO header.
  TndoHeader *th = malloc(sizeof(TndoHeader));
  memcpy(&th->magic, TNDO_MAGIC, sizeof(uint32_t));
  th->type = htonl(TNDO_TYPE_AUDIO);
  th->compression = htonl(TNDO_COMPRESSION_DDPCM);
  th->sampleRate = htonl(wh->sampleRate);
  th->numChannels = htonl(wh->numChannels);
  th->bitsPerSample = htonl(wh->bitsPerSample);

  fwrite(th, sizeof(TndoHeader), 1, outfile);

  // Save compressed data.
  // Preamble, in network order:
  // uint32_t numSamples;
  // uint32_t numFrames;
  // uint32_t numQTables;
  // uint32_t framesPerQTable;

  uint32_t numSamplesBE = htonl(ddpcmh->numSamples);
  uint32_t numFramesBE = htonl(ddpcmh->numFrames);
  uint32_t numQTablesBE = htonl(ddpcmh->numQTables);
  uint32_t framesPerQTableBE = htonl(ddpcmh->framesPerQTable);

  fwrite(&numSamplesBE, sizeof(uint32_t), 1, outfile);
  fwrite(&numFramesBE, sizeof(uint32_t), 1, outfile);
  fwrite(&numQTablesBE, sizeof(uint32_t), 1, outfile);
  fwrite(&framesPerQTableBE, sizeof(uint32_t), 1, outfile);

  // q_table data is stored in network order.
  for (uint32_t i = 0; i < ddpcmh->numQTables; i++) {
    int16_t *ql = ddpcmh->qtablesLeft[i];
    int16_t *qr = ddpcmh->qtablesRight[i];
    for (uint32_t j = 0; j < DDPCM_QTABLE_ENTRIES; j++) {
      ql[j] = htons(ql[j]);
      qr[j] = htons(qr[j]);
    }
  }

  for (int i = 0; i < ddpcmh->numQTables; i++) {
    fwrite(ddpcmh->qtablesLeft[i], sizeof(int16_t), 64, outfile);
  }

  for (int i = 0; i < ddpcmh->numQTables; i++) {
    fwrite(ddpcmh->qtablesRight[i], sizeof(int16_t), 64, outfile);
  }

  fwrite(ddpcmh->scalesLeft, ddpcmh->numFrames, 1, outfile);
  fwrite(ddpcmh->scalesRight, ddpcmh->numFrames, 1, outfile);

  fwrite(ddpcmh->left, ddpcmh->numFrames, DDPCM_COMPRESSED_FRAME_SIZE, outfile);
  fwrite(ddpcmh->right, ddpcmh->numFrames, DDPCM_COMPRESSED_FRAME_SIZE,
         outfile);

  fclose(outfile);

  exit(0);
}
