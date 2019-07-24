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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "ddpcm.h"
#include "quantiser.h"

static inline void pack8to6(uint8_t *src, uint8_t *dst) {
	uint32_t packed1 = 0;
	uint16_t packed2 = 0;
	
	packed1 = (((uint32_t) *src++) & 0x3f) << 26;
	packed1 |= (((uint32_t) *src++) & 0x3f) << 20;
	packed1 |= (((uint32_t) *src++) & 0x3f) << 14;
	packed1 |= (((uint32_t) *src++) & 0x3f) << 8;
	packed1 |= (((uint32_t) *src++) & 0x3f) << 2;
	packed1 |= (((uint32_t) *src) & 0x3f) >> 4;
	
	packed2 = (((uint16_t) *src++) & 0xf) << 12;
	packed2 |= (((uint16_t) *src++) & 0x3f) << 6;
	packed2 |= (((uint16_t) *src++) & 0x3f);

	*dst++ = (uint8_t) ((packed1 >> 24) & 0xff);
	*dst++ = (uint8_t) ((packed1 >> 16) & 0xff);
	*dst++ = (uint8_t) ((packed1 >> 8) & 0xff);
	*dst++ = (uint8_t) (packed1 & 0xff);

	*dst++ = (uint8_t) ((packed2 >> 8) & 0xff);
	*dst = (uint8_t) (packed2 & 0xff);
	
}

static inline void unpack6to8(uint8_t *src, uint8_t *dst) {
	uint32_t unpacked1 = 0;
	uint32_t unpacked2 = 0;

	unpacked1 = (((uint32_t) *src) >> 2) << 24;
	unpacked1 |= (((uint32_t) *src++) & 0x3) << 20;
	unpacked1 |= (((uint32_t) *src) >> 4) << 16;
	unpacked1 |= (((uint32_t) *src++) & 0xf) << 10;
	unpacked1 |= (((uint32_t) *src) >> 6) << 8;
	unpacked1 |= (((uint32_t) *src++) & 0x3f);

	unpacked2 = (((uint32_t) *src) >> 2) << 24;
	unpacked2 |= (((uint32_t) *src++) & 0x3) << 20;
	unpacked2 |= (((uint32_t) *src) >> 4) << 16;
	unpacked2 |= (((uint32_t) *src++) & 0xf) << 10;
	unpacked2 |= (((uint32_t) *src) >> 6) << 8;
	unpacked2 |= (((uint32_t) *src++) & 0x3f);

	*dst++ = (uint8_t)  ((unpacked1 >> 24) & 0xff);
	*dst++ = (uint8_t)  ((unpacked1 >> 16) & 0xff);
	*dst++ = (uint8_t)  ((unpacked1 >> 8) & 0xff);
	*dst++ = (uint8_t)  (unpacked1 & 0xff);

	*dst++ = (uint8_t)  ((unpacked2 >> 24) & 0xff);
	*dst++ = (uint8_t)  ((unpacked2 >> 16) & 0xff);
	*dst++ = (uint8_t)  ((unpacked2 >> 8) & 0xff);
	*dst = (uint8_t)  (unpacked2 & 0xff);


}


// Pick best delta from the q_table and minimise error.
static inline uint32_t bestMatch(int16_t y1, int16_t y2, int16_t to, int16_t *q_table, int16_t scale) {
	uint8_t best;
	uint16_t bestDistance = USHRT_MAX;

	int16_t predicted = predict(y1, y2);
	
	for(uint32_t i = 0; i < 63; i++) {
		uint16_t distance;
		uint16_t t,f ;
		
		t = to + 32768;
		f = (predicted + 32768) + (q_table[i] / scale);
		if(f >= t) {
			distance = f - t;
		} else {
			distance = t - f;
		}

		if(distance < bestDistance) {
				bestDistance = distance;
				best = i;
		}
		
	}

	return (bestDistance << 16) | best;
}


// Encodes a single frame from src to dst using the provided q_table.
// Returns the scaling factor that produces the smallest cumulative error.
uint8_t encodeFrame(int16_t *src, uint8_t *dst, int16_t *q_table) {
	uint8_t bestScale = 1;
	uint32_t bestError = UINT32_MAX;
	
	for(uint32_t sc = 1; sc < DDPCM_MAX_SCALING; sc++) {
		int16_t buffer[DDPCM_FRAME_NUMSAMPLES - 2];
		uint8_t unpacked[DDPCM_FRAME_NUMSAMPLES - 2];
		int16_t y1, y2, p, to;
		int16_t prev1, prev2;
		int16_t *first = (int16_t *) dst;
		uint32_t res;
		uint32_t error;
		
		uint8_t scale;
		
		scale = sc;
		error = 0;
  
		first[0] = htons(src[0]);
		first[1] = htons(src[1]);
		
		y1 = src[0];
		y2 = src[1];
		to = src[2];
		res = bestMatch(y1, y2, to, q_table, scale);
		error += res >> 16;
		unpacked[0] = res & 0xff;
		p = predict(y1, y2);
		buffer[0] = p + (q_table[unpacked[0]] / scale);
		
		y1 = src[1];
		y2 = buffer[0];
		to = src[3];
		res = bestMatch(y1, y2, to, q_table, scale);
		error += res >> 16;
		unpacked[1] = res & 0xff;
		p = predict(y1, y2);
		buffer[1] = p + (q_table[unpacked[1]] / scale);
		
		for(uint32_t i = 4; i < DDPCM_FRAME_NUMSAMPLES; i++) {
			y1 = buffer[i - 4];
			y2 = buffer[i - 3];
			to = src[i];
			res = bestMatch(y1, y2, to, q_table, scale);
			error += res >> 16;
			unpacked[i - 2] = res & 0xff;
			p = predict(y1, y2);
			buffer[i - 2] = p + (q_table[unpacked[i - 2]] / scale);
		}
		
		if(error < bestError) {
			bestScale = scale;
			bestError = error;
		}
	}

	
	// Compress with the best scaling...
	int16_t buffer[DDPCM_FRAME_NUMSAMPLES - 2];
	uint8_t unpacked[DDPCM_FRAME_NUMSAMPLES - 2];
	int16_t y1, y2, p, to;
	int16_t prev1, prev2;
	int16_t *first = (int16_t *) dst;
	uint32_t res;
	uint32_t error = 0;
	
	first[0] = htons(src[0]);
	first[1] = htons(src[1]);
	
	y1 = src[0];
	y2 = src[1];
	to = src[2];
	res = bestMatch(y1, y2, to, q_table, bestScale);
	error += res >> 16;
	unpacked[0] = res & 0xff;
	p = predict(y1, y2);
	buffer[0] = p + (q_table[unpacked[0]] / bestScale);
		
	y1 = src[1];
	y2 = buffer[0];
	to = src[3];
	res = bestMatch(y1, y2, to, q_table, bestScale);
	error += res >> 16;
	unpacked[1] = res & 0xff;
	p = predict(y1, y2);
	buffer[1] = p + (q_table[unpacked[1]] / bestScale);
		
	for(uint32_t i = 4; i < DDPCM_FRAME_NUMSAMPLES; i++) {
		y1 = buffer[i - 4];
		y2 = buffer[i - 3];
		to = src[i];
		res = bestMatch(y1, y2, to, q_table, bestScale);
		error += res >> 16;
		unpacked[i - 2] = res & 0xff;
		p = predict(y1, y2);
		buffer[i - 2] = p + (q_table[unpacked[i - 2]] / bestScale);
	}
		
	pack8to6(unpacked, &dst[4]);
	pack8to6(&unpacked[8], &dst[10]);
	pack8to6(&unpacked[16], &dst[16]);
	pack8to6(&unpacked[24], &dst[22]);
	pack8to6(&unpacked[32], &dst[28]);
	pack8to6(&unpacked[40], &dst[34]);
		
	return bestScale;
  
}

// Decodes a single frame from src to dst using the provided q_table.
void decodeFrame(uint8_t *src, int16_t *dst, int16_t *q_table, uint8_t scale) {
	uint8_t unpacked[DDPCM_FRAME_NUMSAMPLES - 2];
	int16_t y1, y2, p;
	
	uint16_t *first = (uint16_t *) src;
	dst[0] = ntohs((int16_t) first[0]);
	dst[1] = ntohs((int16_t) first[1]);
	
	unpack6to8(&src[4], unpacked);
	unpack6to8(&src[10], &unpacked[8]);
	unpack6to8(&src[16], &unpacked[16]);
	unpack6to8(&src[22], &unpacked[24]);
	unpack6to8(&src[28], &unpacked[32]);
	unpack6to8(&src[34], &unpacked[40]);
	
	for(uint32_t i = 2; i < DDPCM_FRAME_NUMSAMPLES; i++) {
	  y1 = dst[i - 2];
	  y2 = dst[i - 1];
	  p = predict(y1, y2);
	  dst[i] = p + (q_table[unpacked[i - 2]] / scale);
	}
}

// Calculates the number of frames that share a single q_table.
uint32_t framesPerQtable(uint32_t numFrames, uint32_t maxTables, uint32_t minTables) {
	uint32_t fpqt = 0;
	
	for(uint32_t i = maxTables; i >= minTables; i--) {
		if((numFrames % i) == 0) {
			fpqt = numFrames / i;
			break;
		}
	}

	return fpqt;
}
