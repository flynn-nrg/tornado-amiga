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

#ifndef INCLUDE_ZOOM_UTIL_H
#define INCLUDE_ZOOM_UTIL_H

#ifndef ZOOM_MIPMAPS
#define ZOOM_MIPMAPS 8
#endif

#define ZOOM_TXT_SIZE 256

int *histogram(unsigned char *, int);
int bucket(int *, int);
float clamp(float, float, float);
float smoothstep(float, float, float);
unsigned char *downSample(unsigned char *, int);

#endif
