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

#ifndef TNDO_SDL_AUDIO_H
#define TNDO_SDL_AUDIO_H

#include "tornado_settings.h"

#define REPLAY_MODE_RAW 1
#define REPLAY_MODE_MUS 2

#define DEFAULT_MUSIC_VOLUME 128

void Audio_ChannelPause();

// Returns 1 if music is playing.
int Audio_ChannelIsActive();

// Returns 0 on success, 1 on failure.
int Audio_Init(demoParams *dp, uint32_t offset);

// Returns 0 on success, 1 on failure.
int Audio_StreamCreateFile(const char *fileName);

// Sets the stream position to the specified millisecond mark.
void Audio_ChannelSetPosition(double millis);

// Plays or resumes the music.
void Audio_ChannelPlay();

void Audio_Start();
void Audio_Close();

// Set music volume.
void Audio_SetVolume(int volume);

#endif
