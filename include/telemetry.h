#ifndef TELEMETRY_H
#define TELEMETRY_H

typedef struct {
  int numSamples;
  int usedSamples;
  float *samples;
} TelemetryData;

TelemetryData **allocateTelemetry(int, int);
int appendTelemetry(float, TelemetryData *);
int saveTelemetry(const char *, TelemetryData *);
int saveCombinedTelemetry(const char *, TelemetryData **, int, const char *);

#endif
