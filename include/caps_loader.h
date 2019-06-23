#ifndef CAPS_LOADER_H
#define CAPS_LOADER_H

typedef struct {
	char *left_buffer;
	char *right_buffer;
} capsData_t;

// Decode a CAPS encoded data stream.
// File is opened by the caller.
capsData_t * loadCaps(FILE *, uint32_t, uint32_t, uint32_t, uint32_t, int);

#endif
