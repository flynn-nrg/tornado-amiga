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

#include "memory.h"
#include "telemetry.h"

// Allocate an arbitrary number of telemetry tracks.
// We explicitly do not want to use tndo_malloc here
// as this data does not count towards the demo totals.
TelemetryData **allocateTelemetry(int num_tracks, int num_samples) {
  TelemetryData **t = (TelemetryData **)tndo_malloc(
      num_tracks * sizeof(TelemetryData *), TNDO_PRIVATE_MEM);

  for (int i = 0; i < num_tracks; i++) {
    t[i] =
        (TelemetryData *)tndo_malloc(sizeof(TelemetryData), TNDO_PRIVATE_MEM);
    t[i]->samples =
        (float *)tndo_malloc(num_samples * sizeof(float), TNDO_PRIVATE_MEM);
    t[i]->numSamples = num_samples;
  }

  return t;
}

// Append an element to the elemetry track. Returns 1 if we have
// exhausted the allocated space.
int appendTelemetry(float value, TelemetryData *t) {
  if (t->usedSamples >= t->numSamples) {
    return 1;
  }

  t->samples[t->usedSamples] = value;
  t->usedSamples++;

  return 0;
}

// Save the telemetry data to the named file.
// Returns 0 if ok, 1 if an error occurred.
int saveTelemetry(const char *fileName, TelemetryData *t) {
  FILE *f = fopen(fileName, "w");
  if (!f) {
    fprintf(stderr, "FATAL - Cannot open file <%s> for writing.\n", fileName);
    return 1;
  }

  for (int i = 0; i < (t->usedSamples - 1); i++) {
    fprintf(f, "%f\n", t->samples[i]);
  }

  fclose(f);
  return 0;
}

// Save combined telemetry data to the named file.
// The footer should contain gnuplot commands to
// output the graph.
// All tracks must have the same number of samples.
// Returns 0 if ok, 1 if an error occurred.
int saveCombinedTelemetry(const char *fileName, TelemetryData **t,
                          int num_tracks, const char *footer) {
  FILE *f = fopen(fileName, "w");
  if (!f) {
    fprintf(stderr, "FATAL - Cannot open file <%s> for writing.\n", fileName);
    return 1;
  }

  if (footer) {
    fprintf(f, "$data << EOD\n");
  }

  for (int i = 0; i < (t[0]->usedSamples - 1); i++) {
    for (int j = 0; j < (num_tracks - 1); j++) {
      fprintf(f, "%f,", t[j]->samples[i]);
    }
    fprintf(f, "%f\n", t[num_tracks - 1]->samples[i]);
  }

  if (footer) {
    fprintf(f, "EOD\n\n");
    fprintf(f, "%s", footer);
  }

  fclose(f);
  return 0;
}
