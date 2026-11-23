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

// Musashi configuration for Tornado's 68k emulation (TORNADO_68K_EMU).
// It replaces Musashi's m68kconf.h: the build passes
// -DMUSASHI_CNF=\"m68k_emu_conf.h\" when it compiles the Musashi sources.

#ifndef M68KCONF__HEADER
#define M68KCONF__HEADER

#define M68K_OPT_OFF 0
#define M68K_OPT_ON 1
#define M68K_OPT_SPECIFY_HANDLER 2

#define M68K_COMPILE_FOR_MAME M68K_OPT_OFF

// We only run in 68040 mode, but Musashi needs the lower models enabled
// to build its opcode tables.
#define M68K_EMULATE_010 M68K_OPT_ON
#define M68K_EMULATE_EC020 M68K_OPT_ON
#define M68K_EMULATE_020 M68K_OPT_ON
#define M68K_EMULATE_030 M68K_OPT_ON
#define M68K_EMULATE_040 M68K_OPT_ON

// Instruction fetches go through m68k_read_immediate_* and
// m68k_read_pcrelative_*. That lets us tell code fetches apart from data
// accesses: running code outside a code region is detected.
#define M68K_SEPARATE_READS M68K_OPT_ON

#define M68K_SIMULATE_PD_WRITES M68K_OPT_OFF

#define M68K_EMULATE_INT_ACK M68K_OPT_OFF
#define M68K_INT_ACK_CALLBACK(A) 0

#define M68K_EMULATE_BKPT_ACK M68K_OPT_OFF
#define M68K_BKPT_ACK_CALLBACK()

#define M68K_EMULATE_TRACE M68K_OPT_OFF

#define M68K_EMULATE_RESET M68K_OPT_OFF
#define M68K_RESET_CALLBACK()

#define M68K_CMPILD_HAS_CALLBACK M68K_OPT_OFF
#define M68K_CMPILD_CALLBACK(v, r)

#define M68K_RTE_HAS_CALLBACK M68K_OPT_OFF
#define M68K_RTE_CALLBACK()

#define M68K_TAS_HAS_CALLBACK M68K_OPT_OFF
#define M68K_TAS_CALLBACK() 1

// Exceptions are handled through the vector table: every vector points into
// the trap page, so no per-exception callbacks are needed.
#define M68K_ILLG_HAS_CALLBACK M68K_OPT_OFF
#define M68K_ILLG_CALLBACK(opcode) 0

#define M68K_TRAP_HAS_CALLBACK M68K_OPT_OFF
#define M68K_TRAP_CALLBACK(trap) 0

#define M68K_EMULATE_FC M68K_OPT_OFF
#define M68K_SET_FC_CALLBACK(A)

#define M68K_MONITOR_PC M68K_OPT_OFF
#define M68K_SET_PC_CALLBACK(A)

// Called before every instruction: return detection, the instruction
// budget watchdog and the PC trace ring.
#define M68K_INSTRUCTION_HOOK M68K_OPT_SPECIFY_HANDLER
void tndo_m68k_instr_hook(unsigned int pc);
#define M68K_INSTRUCTION_CALLBACK(pc) tndo_m68k_instr_hook(pc)

#define M68K_EMULATE_PREFETCH M68K_OPT_OFF

// Odd instruction addresses are detected by our fetch callback.
#define M68K_EMULATE_ADDRESS_ERROR M68K_OPT_OFF

#define M68K_LOG_ENABLE M68K_OPT_OFF
#define M68K_LOG_1010_1111 M68K_OPT_OFF
#define M68K_LOG_TRAP M68K_OPT_OFF
#define M68K_LOG_FILEHANDLE stderr

// Off: it isn't needed, and Musashi does not build with both the PMMU and
// M68K_SEPARATE_READS enabled.
#define M68K_EMULATE_PMMU M68K_OPT_OFF

#define M68K_USE_64_BIT M68K_OPT_ON

#endif // M68KCONF__HEADER
