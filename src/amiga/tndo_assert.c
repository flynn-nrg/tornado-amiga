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

#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "tndo_assert.h"

static char assert_buffer[1024];

void __tndo_assert(const char *msg, const char *file, int line) {
  snprintf(assert_buffer, 1023, "Assertion failed: %s, file %s, line %i.\n",
           msg, file, line);
  serialLog(assert_buffer);
  abort();
}
