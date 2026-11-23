/*
Copyright (c) 2026, Miguel Mendez. All rights reserved.

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

// Interface between the emulator core (m68k_emu.c) and the hunk loader and
// linker (m68k_emu_hunk.c). Not part of the public API.

#ifndef M68K_EMU_INTERNAL_H
#define M68K_EMU_INTERNAL_H

#include "tndo_m68k.h"

// Every external symbol gets a thunk address in the trap page. Executing it
// calls the bound host function; reading it is an UNRESOLVED fault.
#define M68K_THUNK_BASE (TNDO_M68K_TRAP_BASE + 0x1000)
#define M68K_MAX_IMPORTS 4096
#define M68K_THUNK_END (M68K_THUNK_BASE + M68K_MAX_IMPORTS * 4)

// Core.
void m68k_emu_ensure_init(void);
void m68k_emu_add_symbol(const char *name, uint32_t addr, uint32_t size,
                         int exported);
uint32_t m68k_emu_find_export(const char *name, int *found);

// Loader: loads the registered modules into a freshly initialized emulator.
// 0 = ok.
int m68k_hunk_load_registered(void);
// Loader: drops all import state (shutdown).
void m68k_hunk_reset(void);
// Loader: thunk index -> symbol name and binding. fn is NULL if unbound.
const char *m68k_hunk_import(int idx, tndo_m68k_host_fn *fn, void **user);

#endif // M68K_EMU_INTERNAL_H
