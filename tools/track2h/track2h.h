#ifndef TRACK_2_H_H
#define TRACK_2_H_H

enum key_type {
	KEY_STEP,   /* stay constant */
	KEY_LINEAR, /* lerp to the next value */
	KEY_SMOOTH, /* smooth curve to the next value */
	KEY_RAMP,
	KEY_TYPE_COUNT
};

struct track_key {
	int row;
	float value;
	enum key_type type;
};

struct sync_track {
	char *name;
	struct track_key *keys;
	int num_keys;
};

struct sync_track * loadTrackData(FILE *);
void printHeader(int, double, char *, FILE *);
void parseAndWrite(struct sync_track *, int, double, char *, FILE *);

#endif
