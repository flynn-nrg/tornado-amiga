/*
 * Converts a wav file to splash sample.
 * Created by Miguel Mendez
 * This file is public domain.
 * Version 1.0 11 November 2018
 */

#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  int format;
  int numChannels;
  int sampleRate;
  int bitsPerSample;
  int *data;
  int dataLen;
} WavHeader;

#define WAV_PCM 1

static WavHeader *parseWavHeader(void *data) {
  int *iPtr = (int *)data;
  char *cPtr = (char *)data;

  // RIFF?
  if (iPtr[0] != 0x46464952) {
    fprintf(stderr, "FATAL - This is not a WAV file!\n");
    return 0;
  }

  // WAVE?
  if (iPtr[2] != 0x45564157) {
    fprintf(stderr, "FATAL - This is not a WAV file!\n");
    return 0;
  }

  // Search for "fmt " subchunk
  int delta = 12;
  int skip = 0;
  int foundFmt = 0;
  unsigned int *p;

  cPtr += delta;

  do {
    p = (unsigned int *)cPtr;
    if (p[0] == 0x20746d66) {
      foundFmt = 1;
      break;
    }
    // Skip to the next subchunk.
    skip = p[1];
    delta += skip + 8;
    cPtr += skip + 8;

    // Give up after 1024 bytes.
  } while (delta < 1024);

  if (!foundFmt) {
    fprintf(stderr, "FATAL - fmt subchunk not found!\n");
    return 0;
  }

  iPtr = (int *)cPtr;
  int audioFormat = iPtr[2] & 0xff;
  if (audioFormat != WAV_PCM) {
    fprintf(stderr, "FATAL - Only PCM is supported!\n");
    return 0;
  }

  int numChannels = (iPtr[2] & 0xff0000) >> 16;
  int sampleRate = iPtr[3];
  int bitsPerSample = (iPtr[5] & 0xff0000) >> 16;

  // "data"
  if (iPtr[6] != 0x61746164) {
    fprintf(stderr, "FATAL - data subchunk not found %x!\n", iPtr[6]);
    return 0;
  }

  WavHeader *wh = malloc(sizeof(WavHeader));
  wh->format = audioFormat;
  wh->numChannels = numChannels;
  wh->sampleRate = sampleRate;
  wh->bitsPerSample = bitsPerSample;
  wh->data = (iPtr + 8);
  wh->dataLen = iPtr[7];

  return wh;
}

static void usage(char *progname) {

  fprintf(stderr, "Usage: %s -i <input_file> -o <output_file>\n", progname);
  exit(EXIT_FAILURE);
}

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

// Amiga samples are signed.
static void fixSamples(WavHeader *wh) {
  char c;
  char *data_c = (char *)wh->data;

  for (int i = 0; i < wh->dataLen; i++) {
    c = data_c[i];
    c = c - 128;
    data_c[i] = c;
  }
}
static int writeSampleFile(WavHeader *wh, FILE *outfile) {
  uint32_t sampleRate;
  uint32_t size;
  int written;

  sampleRate = htonl(wh->sampleRate);
  size = htonl(wh->dataLen);

  written = fwrite(&sampleRate, sizeof(uint32_t), 1, outfile);
  if (written != 1) {
    fprintf(stderr,
            "FATAL - Could not write sample rate data to output file.\n");
    return -1;
  }

  written = fwrite(&size, sizeof(uint32_t), 1, outfile);
  if (written != 1) {
    fprintf(stderr,
            "FATAL - Could not write sample size data to output file.\n");
    return -1;
  }

  written = fwrite(wh->data, wh->dataLen, 1, outfile);
  if (written != 1) {
    fprintf(stderr, "FATAL - Could not write sample data to output file.\n");
    return -1;
  }

  return 0;
}

int main(int argc, char **argv) {
  int ch;
  FILE *infile, *outfile;

  while ((ch = getopt(argc, argv, "i:o:")) != -1) {
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
    case '?':
    default:
      usage(argv[0]);
    }
  }
  argc -= optind;
  argv += optind;

  void *data = loadFile(infile);
  WavHeader *wh = parseWavHeader(data);

  if (!wh) {
    exit(EXIT_FAILURE);
  }

  if (wh->numChannels != 1) {
    fprintf(stderr, "FATAL - Only mono wav files supported. Exiting.\n");
    exit(EXIT_FAILURE);
  }

  printf("Sample information:\n");
  printf("Bits per sample: %i\n", wh->bitsPerSample);
  printf("Sample rate: %i\n", wh->sampleRate);
  printf("Size: %i\n", wh->dataLen);

  fixSamples(wh);

  writeSampleFile(wh, outfile);

  fclose(infile);
  fclose(outfile);

  free(data);
  free(wh);

  exit(0);
}
