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
#include <unistd.h>

#define OP_COMPUTE 1
#define OP_ADJUST 2

#define ZOOM_TXT_SIZE 256
#define UPPER_LIMIT 15
#define LOWER_LIMIT 0

void resetHistogram(int *h) {
  for (int i = 0; i < 255; i++) {
    h[i] = 0;
  }
}

void histogram(unsigned char *canvas, int *h, int size) {
  int p;
  unsigned char c;

  // Reset histogram first as we reuse it.
  resetHistogram(h);

  for (int i = 0; i < size; i++) {
    c = canvas[i];
    p = (int)c;
    h[p]++;
  }
}

int bucket(int *histogram, int size) {
  int b = 0;

  // Let's pretend 0 is 1.
  // b += histogram[0];

  for (int i = 1; i < 256; i++) {
    b += histogram[i] * i;
  }

  return b / size;
}

void brighten(unsigned char *canvas, int size, int limit) {
  for (int i = 0; i < size; i++) {
    if (canvas[i] < limit) {
      canvas[i] += 1;
    }
  }
}

void darken(unsigned char *canvas, int size, int limit) {
  for (int i = 0; i < size; i++) {
    if (canvas[i] > limit) {
      canvas[i] -= 1;
    }
  }
}

void adjust(unsigned char *canvas, int *hist, int wantedBucket, int size) {
  int b;

  histogram(canvas, hist, size);
  b = bucket(hist, ZOOM_TXT_SIZE * ZOOM_TXT_SIZE);
  while (b != wantedBucket) {
    if (b < wantedBucket) {
      brighten(canvas, size, UPPER_LIMIT);
    } else {
      darken(canvas, size, LOWER_LIMIT);
    }

    histogram(canvas, hist, size);
    b = bucket(hist, ZOOM_TXT_SIZE * ZOOM_TXT_SIZE);
  }
}

void usage(char *progName) {
  fprintf(stderr,
          "Usage: %s -[operation] -f file\nWhere operations is one of:\n-a "
          "value : Adjust histogram to desired value and overwrite image.\n-c "
          ": Compute histogram for the given file.\n",
          progName);
}

int main(int argc, char **argv) {
  int b, ch;
  int operation;
  int wantedBucket;
  FILE *inFile;
  int numRead, numWritten;

  while ((ch = getopt(argc, argv, "f:ca:")) != -1) {
    switch (ch) {
    case 'f':
      inFile = fopen(optarg, "r+");
      if (!inFile) {
        fprintf(stderr, "FATAL - Unable to open %s for reading. Aborting.\n",
                optarg);
        exit(1);
      }
      break;
    case 'a':
      operation = OP_ADJUST;
      wantedBucket = atoi(optarg);
      break;
    case 'c':
      operation = OP_COMPUTE;
      break;
    case '?':
    default:
      usage(argv[0]);
    }
  }

  if (!inFile)
    usage(argv[0]);

  argc -= optind;
  argv += optind;

  unsigned char *canvas = calloc(1, ZOOM_TXT_SIZE * ZOOM_TXT_SIZE);
  int *hist = calloc(256 * sizeof(int), 1);

  switch (operation) {
  case OP_COMPUTE:
    numRead = fread(canvas, 1, ZOOM_TXT_SIZE * ZOOM_TXT_SIZE, inFile);
    if (numRead != ZOOM_TXT_SIZE * ZOOM_TXT_SIZE) {
      fprintf(stderr,
              "FATAL: Raw files must be %ix%i in size, 8 bits per pixel. "
              "Aborting.\n",
              ZOOM_TXT_SIZE, ZOOM_TXT_SIZE);
      fclose(inFile);
      free(canvas);
      exit(1);
    }
    histogram(canvas, hist, ZOOM_TXT_SIZE * ZOOM_TXT_SIZE);
    b = bucket(hist, ZOOM_TXT_SIZE * ZOOM_TXT_SIZE);
    printf("Bucket: %i\n", b);
    free(hist);
    break;
  case OP_ADJUST:
    numRead = fread(canvas, 1, ZOOM_TXT_SIZE * ZOOM_TXT_SIZE, inFile);
    if (numRead != ZOOM_TXT_SIZE * ZOOM_TXT_SIZE) {
      fprintf(stderr,
              "FATAL: Raw files must be %ix%i in size, 8 bits per pixel. "
              "Aborting.\n",
              ZOOM_TXT_SIZE, ZOOM_TXT_SIZE);
      fclose(inFile);
      free(canvas);
      exit(1);
    }
    histogram(canvas, hist, ZOOM_TXT_SIZE * ZOOM_TXT_SIZE);
    b = bucket(hist, ZOOM_TXT_SIZE * ZOOM_TXT_SIZE);
    printf("Old bucket: %i. New bucket: %i\n", b, wantedBucket);
    adjust(canvas, hist, wantedBucket, ZOOM_TXT_SIZE * ZOOM_TXT_SIZE);
    rewind(inFile);
    numWritten = fwrite(canvas, 1, ZOOM_TXT_SIZE * ZOOM_TXT_SIZE, inFile);
    if (numRead != ZOOM_TXT_SIZE * ZOOM_TXT_SIZE) {
      fprintf(stderr, "WARNING - Could not write file.\n");
    }
    free(hist);
    break;
  default:
    fprintf(stderr, "Invalid operation: %i\n.", operation);
    exit(1);
  }

  fclose(inFile);
  free(canvas);
}
