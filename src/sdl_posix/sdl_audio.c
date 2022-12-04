/*
Copyright (c) 2022, Miguel Mendez. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "stdlib.h"

#include "sdl_audio.h"

#include "SDL.h"
#include "SDL_mixer.h"
#include "tornado_settings.h"

static int audio_open = 0;
static int music_started = 0;
static int replay_mode = 0;
static int music_volume = 0;
static int paused = 0;
static int modified = 0;
static uint8_t *rawSample;
static uint32_t bytesPerSecond;
static uint32_t sampleLen;

static Mix_Chunk *stream = NULL;
static Mix_Music *music = NULL;
static int streamChannel = 0xdeadbeef;

void Audio_ChannelPause() {
  if (!paused) {
    Mix_Pause(streamChannel);
    paused = 1;
  }
}

static Mix_Chunk *interleave(uint8_t *left, uint8_t *right, uint32_t numSamples,
                             uint32_t bitsPerSample, uint32_t offset) {
  sampleLen = numSamples * (bitsPerSample / 8) * 2;
  rawSample = malloc(sampleLen);
  uint8_t *tmp = rawSample;

  left -= offset;
  right -= offset;

  for (uint32_t i = 0; i < numSamples; i++) {
    *tmp++ = *left++;
    *tmp++ = *left++;
    *tmp++ = *right++;
    *tmp++ = *right++;
  }

  return Mix_QuickLoad_RAW(rawSample, numSamples * bitsPerSample * 2);
}

// Returns 1 if music is playing.
int Audio_ChannelIsActive() { return Mix_Playing(streamChannel); }

int Audio_Init(demoParams *dp, uint32_t offset) {
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    SDL_Log("Couldn't initialize SDL: %s\n", SDL_GetError());
    return (1);
  }

  if (Mix_OpenAudio(dp->sampleRate, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
    SDL_Log("Couldn't open audio: %s\n", SDL_GetError());
    return (1);
  }

  if (Mix_AllocateChannels(4) < 0) {
    SDL_Log("Couldn't allocate channels: %s\n", SDL_GetError());
    return (1);
  }

  bytesPerSecond = dp->sampleRate * (dp->bitsPerSample / 8) * 2;
  stream =
      interleave((uint8_t *)*dp->mixState, (uint8_t *)*dp->mixState2,
                 (uint32_t)dp->numSamples, (uint32_t)dp->bitsPerSample, offset);

  if (stream == NULL) {
    SDL_Log("Couldn't load RAW data into chunk!\n");
    return -1;
  }

  Mix_VolumeChunk(stream, music_volume);

  streamChannel = Mix_PlayChannel(-1, stream, 0);
  Mix_Pause(streamChannel);

  audio_open = 1;
  return 0;
}

int Audio_StreamCreateFile(const char *fileName) {
  music = Mix_LoadMUS(fileName);
  if (music == NULL) {
    SDL_Log("Couldn't load %s: %s\n", fileName, SDL_GetError());
    if (audio_open) {
      Mix_CloseAudio();
      audio_open = 0;
    }
    return 1;
  }

  Mix_PlayMusic(music, 0);
  Mix_PauseMusic();

  return 0;
}

void Audio_ChannelSetPosition(double pos) {
  if (stream) {
    modified = 1;
    Mix_HaltChannel(streamChannel);
    uint32_t offset = (uint32_t)pos * bytesPerSecond;
    stream->abuf = rawSample + offset;
    stream->alen = sampleLen - offset;
    if (!paused) {
      Mix_PlayChannel(streamChannel, stream, 0);
      modified = 0;
    }
  }
}

void Audio_ChannelPlay() {
  paused = 0;
  if (modified) {
    Mix_PlayChannel(streamChannel, stream, 0);
    modified = 0;
  } else {
    Mix_Resume(streamChannel);
  }
}

void Audio_Start() {}

void Audio_Close() {
  if (!audio_open)
    return;

  if (Mix_Playing(streamChannel)) {
    Mix_FadeOutChannel(streamChannel, 200);
  }

  if (audio_open) {
    Mix_CloseAudio();
    audio_open = 0;
    free(rawSample);
    rawSample = NULL;
  }
}

void Audio_SetVolume(int musicVolume) { music_volume = musicVolume; }