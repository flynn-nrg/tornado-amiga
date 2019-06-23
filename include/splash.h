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
