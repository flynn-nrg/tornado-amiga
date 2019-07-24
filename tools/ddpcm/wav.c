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

#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ddpcm.h"
#include "wav.h"

WavHeader *parseWavHeader(void *data) {
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

  WavHeader *wh = calloc(1, sizeof(WavHeader));
  wh->format = audioFormat;
  wh->numChannels = numChannels;
  wh->sampleRate = sampleRate;
  wh->bitsPerSample = bitsPerSample;
  wh->data = (iPtr + 8);
  wh->dataLen = iPtr[7];

  return wh;
}


AudioData *splitChannels(WavHeader *wh) {
	AudioData *ad = calloc(1, sizeof(AudioData));

	ad->bitsPerSample = wh->bitsPerSample;
	ad->numChannels = wh->numChannels;
	ad->sampleRate = wh->sampleRate;
	
	switch(wh->bitsPerSample) {
	case 8:
		ad->sampleSize = 1;
		break;
	case 16:
		ad->sampleSize = 2;
		break;
	default:
		fprintf(stderr, "FATAL - Unsupported sample bitdepth <%i>. Aborthing.\n", wh->bitsPerSample);
		abort();
	}
	
	ad->numSamples = (wh->dataLen / wh->numChannels) / ad->sampleSize;

	uint32_t partial_frame = ad->numSamples % DDPCM_FRAME_NUMSAMPLES;
	uint32_t padding = 0;
	
	if(partial_frame > 0) {
		padding = DDPCM_FRAME_NUMSAMPLES - partial_frame;
	}

	
	uint32_t addSamples = 0;

	// Pad a few extra frames to have an integer amount of frames per q_table.
	for(;;) {
		uint32_t numFrames = (ad->numSamples + padding + addSamples) / DDPCM_FRAME_NUMSAMPLES;
		if(numFrames % DDPCM_MAX_TABLES == 0) {
			break;
		}
		addSamples += DDPCM_FRAME_NUMSAMPLES;
	}

	ad->left = calloc((wh->dataLen / wh->numChannels) + (DDPCM_FRAME_NUMSAMPLES * ad->sampleSize) + (addSamples * ad->sampleSize), 1);
	ad->right = calloc((wh->dataLen / wh->numChannels) + (DDPCM_FRAME_NUMSAMPLES * ad->sampleSize)  + (addSamples * ad->sampleSize), 1);
	
	uint32_t r_index = 0;
	uint32_t w_index = 0;
	int16_t *left = (int16_t *) ad->left;
	int16_t *right = (int16_t *) ad->right;
	int16_t *payload = (int16_t *) wh->data;
	
	for(uint32_t i = 0; i < ad->numSamples; i++) {
		left[i] = payload[r_index++];
		right[i] = payload[r_index++];
	}

	ad->numSamples += padding;
	ad->numSamples += addSamples;
	
	return ad;

}

WavHeader *joinChannels(AudioData *ad) {
	WavHeader *wh = calloc(1, sizeof(WavHeader));
	wh->format = WAV_PCM;
	wh->numChannels = ad->numChannels;
	wh->sampleRate = ad->sampleRate;
	wh->bitsPerSample = ad->bitsPerSample;
	wh->data = calloc(ad->numSamples * ad->numChannels * ad->sampleSize, 1);
	wh->dataLen = ad->numSamples * ad->numChannels * ad->sampleSize;

	int16_t *payload = (int16_t *) wh->data;
	int16_t *left = (int16_t *) ad->left;
	int16_t *right = (int16_t *) ad->right;

	uint32_t offset = 0;
	for(uint32_t i = 0; i < ad->numSamples; i++) {
		payload[offset++] = left[i];
		payload[offset++] = right[i];
	}

	return wh;

}

// Save a WAV file with the data. This code is *LITTLE ENDIAN* only!
int writeWAV(FILE *output, WavHeader *wh) {
	uint32_t *header = calloc(1, WAVE_HEADER_SIZE);
	uint16_t *header16 = (uint16_t *) header;
	
	header[0] = 0x46464952; // RIFF magic
	header[1] = WAVE_HEADER_SIZE + wh->dataLen - 8; // Chunk size
	header[2] = 0x45564157; // WAVE magic
	
	header[3] = 0x20746d66; // fmt. subchunk
	header[4] = 16; // 16 for PCM data

	header16[10] = WAV_PCM; // PCM data
	header16[11] = wh->numChannels; // NumChannels

	header[6] = wh->sampleRate; // SampleRate
	header[7] = wh->sampleRate * wh->numChannels * (wh->bitsPerSample / 8); // ByteRate

	header16[16] = wh->numChannels * (wh->bitsPerSample / 8); // BlockAlign
	header16[17] = wh->bitsPerSample; //BitsPerSample

	header[9] = 0x61746164; // data magic
	header[10] = wh->dataLen; // data size

	int written = fwrite(header, WAVE_HEADER_SIZE, 1, output);
	if(!written) {
		fprintf(stderr, "FATAL - Could not write wave header to output file.\n");
		return -1;
	}

	written = fwrite(wh->data, wh->dataLen, 1, output);
	if(!written) {
		fprintf(stderr, "FATAL - Could not write wave data to output file.\n");
		return -1;
	}
	
	
	return 0;
}
