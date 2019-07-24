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

#include "quantiser.h"

// Linear extrapolation predictor.
// y = y1 + (y2 - y1) * t.
int16_t predict(int16_t y1, int16_t y2) {
  int32_t t = 2;
  int32_t y132 = (int32_t)y1;
  int32_t y232 = (int32_t)y2;

  int32_t p = y132 + ((y232 - y132) * t);
  if (p > INT16_MAX)
    return INT16_MAX / 2;
  if (p < INT16_MIN)
    return INT16_MIN / 2;
  return (int16_t)p;
}

// Compute the delta histogram of an array of 16bit signed samples.
static uint32_t *histogram16(int16_t *data, uint32_t numSamples) {
  uint32_t *hist = calloc(65536, sizeof(uint32_t));

  for (uint32_t i = 1; i < (numSamples - 2); i++) {
    int16_t p1 = predict(data[i - 1], data[i]);
    int16_t d1 = data[i];
    int16_t d2 = data[i + 1];
    int16_t d3 = d2 - d1;
    int16_t predicted = p1 + 32768;
    uint16_t index = (uint16_t)(d3 - p1) + 32768;
    hist[index]++;
  }

  return hist;
}

static int deltaCmpFunc16(const void *a, const void *b) {
  q_entry *qa = (q_entry *)a;
  q_entry *qb = (q_entry *)b;
  if (qa->score > qb->score) {
    return -1;
  } else if (qa->score < qb->score) {
    return 1;
  }

  return 0;
}

// Sort the most frequently used deltas inside of each bucket.
static q_entry **sortDeltas16(uint32_t *histogram) {
  q_entry **entries = calloc(64, sizeof(q_entry *));
  for (uint32_t i = 0; i < 64; i++) {
    entries[i] = calloc(1024, sizeof(q_entry));
  }

  for (uint32_t i = 0; i < 64; i++) {
    for (uint32_t j = 0; j < 1024; j++) {
      entries[i][j].score = histogram[(i * 1024) + j];
      entries[i][j].value = (int16_t)(((i * 1024) + j) - 32768);
    }
  }

  for (uint32_t i = 0; i < 64; i++) {
    qsort(&entries[i][0], 1024, sizeof(q_entry), deltaCmpFunc16);
  }

  return entries;
}

static int qtableCmpFunc16(const void *a, const void *b) {
  int16_t *ia = (int16_t *)a;
  int16_t *ib = (int16_t *)b;
  if (*ia < *ib) {
    return -1;
  } else if (*ia > *ib) {
    return 1;
  }

  return 0;
}

// Takes the array of sorted deltas and outputs a 64 entry array of quantised
// values. Each value is the wieghted sum of all the deltas in that bucket.
static int16_t *quantise16Weighted(q_entry **sortedDeltas) {

  int16_t *q_table = calloc(64, sizeof(int16_t));

  for (uint32_t i = 0; i < 64; i++) {
    float weighted = 0.0f;
    float numSamples = 0.0f;

    for (uint32_t j = 0; j < 1024; j++) {
      numSamples += (float)sortedDeltas[i][j].score;
    }

    if (numSamples > 0.0) {
      for (uint32_t j = 0; j < 1024; j++) {
        weighted += (((float)sortedDeltas[i][j].value) *
                     ((float)sortedDeltas[i][j].score)) /
                    numSamples;
      }

      q_table[i] = (int16_t)weighted;
    }
  }

  return q_table;
}

// Takes the arrays of sorted deltas and outputs a 64 entry array of quantised
// values. The more often a value is present the more weight it has on the
// quatised entry.
static int16_t *quantise16Top(q_entry **sortedDeltas) {

  uint32_t index = 0;

  int16_t *q_table = calloc(64, sizeof(int16_t));

  for (uint32_t i = 0; i < 1024; i++) {
    for (uint32_t j = 0; j < 64; j++) {
      if (sortedDeltas[j][i].score > 0) {
        q_table[index] = sortedDeltas[j][i].value;
        index++;
      }
      if (index > 63)
        break;
    }
    if (index > 63)
      break;
  }

  qsort(q_table, 64, sizeof(int16_t), qtableCmpFunc16);
  return q_table;
}

int16_t *quantiser(int16_t *data, uint32_t numSamples, uint32_t deltaMode) {
  int16_t *q_table;

  uint32_t *hist = histogram16(data, numSamples);
  q_entry **q_entries = sortDeltas16(hist);

  switch (deltaMode) {
  case DELTA_MODE_WEIGHTED:
    q_table = quantise16Weighted(q_entries);
    break;
  case DELTA_MODE_TOP:
    q_table = quantise16Top(q_entries);
    break;
  default:
    fprintf(stderr, "FATAL - Invalid delta mode <%u> specificed. Aborting.\n",
            deltaMode);
    abort();
  }

  return q_table;
}
