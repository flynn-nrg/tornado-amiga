/*
Copyright (c) 2019, Miguel Mendez. All rights reserved.

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

#ifndef SPLASH_H
#define SPLASH_H

void splash_bg_init(void);
void splash_bg_end(void);
void splash_end(void);
void splash_show(void);
void splash_black_pal(uint32_t *);
void splash_fadein(uint32_t *, int);
void splash_fadeout(uint32_t *, int);
int splash_checkinput(void);
void splash_swap_screens(void);
void splash_forefront(void);
int splash_load_sample(const char *, unsigned int);
void splash_play_sample(void);
void splash_end_sample(void);
void splash_set_palette(void *);
void splash_wait_vbl(void);
int splash_load_mod(const char *, unsigned int);
void splash_play_mod(void);
void splash_end_mod(void);
unsigned char *splash_init(const char *const *, int, int, int, int, int, int);

#endif
