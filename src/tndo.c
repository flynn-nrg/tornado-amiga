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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "ptr_bridges.h"
#include "tndo.h"
#include "tndo_file.h"

static TndoHeader th;

// Try to parse the TNDO header. File is assumed to be already open by the
// caller.
TndoHeader *openTNDO(FILE *fd) {

  int read = tndo_fread(&th, sizeof(TndoHeader), 1, fd);
  if (read != 1) {
    printf("FATAL - Cannot read TNDO header.\n");
    return 0;
  }

  // Check for TNDO signature.
  if (th.magic != (ENDI4(TNDO_MAGIC_INT))) {
    tndo_fseek(fd, 0, SEEK_SET);
    return 0;
  }

  // Return pointer to TNDO struct.
  return &th;
}
