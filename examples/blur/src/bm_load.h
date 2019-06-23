/*
Copyright (c) 2019 Luis Pons
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

#ifndef BM_LOAD_H
#define BM_LOAD_H

#define BM_QUA10      (1)
#define BM_GREY5      (2)
#define BM_NORM16     (3)
#define BM_RGB23      (4)
#define BM_QUA8       (5)
#define BM_GREY4      (6)
#define BM_BILEVEL    (7)
#define BM_GREY8      (8)
#define BM_GREY_ALPHA (9)
#define BM_GREY6      (10)

// Preallocated: optional memory buffer. If ==0, tndo_malloc is used

void bm_load (t_canvas* c, void* data, void* preallocated, int preallocated_size);

#endif
