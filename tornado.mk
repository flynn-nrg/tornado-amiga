#################################################################################
# Do NOT modify this makefile directly!!!
#################################################################################

#################################################################################

# Tornado core functionality
TORNADO_OBJ = amiga/startup.o amiga/cpu.o amiga/aga.o 
TORNADO_OBJ += memory.o amiga/system.o debug.o amiga/tndo_assert.o
TORNADO_OBJ += hardware_check.o amiga/chrono.o amiga/freq_estimation.o 
TORNADO_OBJ += telemetry.o prof.o amiga/time.o dprint.o
TORNADO_OBJ += amiga/audio.o amiga/paula_output.o
TORNADO_OBJ += amiga/audio_lowlevel.o
TORNADO_OBJ += placeholder.o

# Intro mode allows you to selectively enable the tornado modules you need
# and save some bytes.
ifndef TORNADO_INTRO_MODE
TORNADO_ASSET_MANAGER = true
TORNADO_GRAPHICS = true
TORNADO_SPLASH = true
TORNADO_AHI = true
TORNADO_P61 = true
TORNADO_DDPCM = true
TORNADO_PRETRACKER = true
TORNADO_CINTER = true
endif

# Asset management and data unpackers.
ifdef TORNADO_ASSET_MANAGER
CCFLAGS += -DTORNADO_ASSET_MANAGER
TORNADO_OBJ += assets.o tndo.o tndo_file.o
TORNADO_OBJ += lzw_loader.o amiga/lzw_unpack.o
TORNADO_OBJ += amiga/lzw_unpack_inner.o
TORNADO_OBJ += lzss_loader.o lzh_loader.o amiga/lzss_unpack.o
TORNADO_OBJ += amiga/lzh_unpack.o lzw_unpack_stream.o lzss_unpack_stream.o
endif

# Graphics subsystem. Disable if using custom displays.
ifdef TORNADO_GRAPHICS
CCFLAGS += -DTORNADO_GRAPHICS
TORNADO_OBJ += amiga/copper.o amiga/graphics.o amiga/c2p1x1_8_c5.o amiga/c2p1x1_8_c5_040_16_9.o
TORNADO_OBJ += amiga/c2p32.o amiga/c2p64.o amiga/c2p1x1_6_c5_040.o amiga/c2p1x1_8_c5_040.o
TORNADO_OBJ += amiga/c2p1x1_8_c5_040_scanline.o amiga/c2p1x1_4_c5_16_9.o
TORNADO_OBJ += amiga/c2p1x1_8_c5_bm.o amiga/c2p1x1_4_c5_16_9_h.o
TORNADO_OBJ += amiga/display.o c2p.o
endif

# Splash screen
ifdef TORNADO_SPLASH
CCFLAGS += -DTORNADO_SPLASH
TORNADO_OBJ += amiga/splash.o
endif

# Audio AHI subsystem.
ifdef TORNADO_AHI
CCFLAGS += -DTORNADO_AHI
TORNADO_OBJ += amiga/audio_ahi.o
endif

# P61 replay routines.
ifdef TORNADO_P61
CCFLAGS += -DTORNADO_P61
TORNADO_OBJ += amiga/mod_replay.o amiga/mod_replay_os_legal.o
endif

# Tornado DDPCM decoder.
ifdef TORNADO_DDPCM
CCFLAGS += -DTORNADO_DDPCM
TORNADO_OBJ += ddpcm_loader.o ddpcm_decode.o amiga/ddpcm_lowlevel.o
endif

# Pretracker replay routine.
ifdef TORNADO_PRETRACKER
TORNADO_OBJ += amiga/prt_replay.o
endif

# Cinter replay rutine.
ifdef TORNADO_CINTER
TORNADO_OBJ += amiga/cinter.o
endif


TORNADO_SRCDIR = $(TORNADO_BASE)/src
TORNADO_THIRD_PARTY_DIR = $(TORNADO_BASE)/third_party

#################################################################################
LZW_BASE = $(TORNADO_BASE)/tools/compress

DDPCM_BASE = $(TORNADO_BASE)/tools/ddpcm
DDPCM_INCDIR = $(DDPCM_BASE)

ROCKET_BASE = $(TORNADO_BASE)/third_party/rocket/lib
ROCKET_INCDIR = $(ROCKET_BASE)

AHI_BASE = $(TORNADO_BASE)/third_party/m68k-amigaos-ahi/Developer
AHI_INCDIR = $(AHI_BASE)/Include/C

CINTER_BASE = $(TORNADO_BASE)/third_party/Cinter
CINTER_INCDIR = $(CINTER_BASE)/player

#################################################################################
# Auto-detect host OS.
UNAME_S := $(shell uname -s)
#################################################################################

INCDIR = $(TORNADO_BASE)/include
ifdef GCC_ELF_HOST
INCDIR += $(TORNADO_BASE)/include_gcc_elf
INCDIR += $(TORNADO_BASE)/include_amiga_math
INCDIR += $(TORNADO_BASE)/third_party/ndk/Include_H
INCDIR += $(TORNADO_BASE)/third_party/ndk/Include_I
INCDIR += $(TORNADO_BASE)/third_party/ndk/
else ifdef LINUX_GCC_HOST
INCDIR += $(TORNADO_BASE)/include_amiga_math
endif
INCDIR += $(TORNADO_BASE)/third_party
INCDIR += $(ROCKET_INCDIR)
INCDIR += $(AHI_INCDIR)
INCDIR += $(LZW_BASE)
INCDIR += $(DDPCM_INCDIR)
INCDIR += $(CINTER_INCDIR)
INCDIR += $(LOCAL_INCDIR)
#################################################################################

OBJECTS = $(TORNADO_OBJ)
OBJECTS += $(LZW_OBJS)
OBJECTS += $(DEMO_OBJS)
OBJECTS += $(DEMO_OBJS_TARGET_AMIGA)

#################################################################################

TARGET   ?= bin/out.exe
SRCDIR   = src
BUILDDIR ?= build
INCDIR   += include
LIBDIR   = lib

################################################################################
# Toolchain selection.
#
# The user sets ONE of these environment variables (or none for VBCC default):
#   GCC_ELF_HOST=true   -> m68k-amiga-elf-gcc (GCC 14, recommended)
#   LINUX_GCC_HOST=true -> m68k-amigaos-gcc (Bebbo 6.x, legacy Linux only)
#   (none)              -> VBCC (default)
#
# The host OS is detected automatically via uname.  On macOS, VBCC/VASM/VLINK
# are expected to be installed via Homebrew.  On Linux, set TOOLCHAIN to the
# directory containing the VBCC/Bebbo cross-compiler.
################################################################################

ifdef GCC_ELF_HOST
# ---------- m68k-amiga-elf-gcc (GCC 14) ----------
CC     := m68k-amiga-elf-gcc

ifeq ($(UNAME_S),Darwin)
  VBCC_PREFIX  := $(shell brew --prefix vbcc)
  VASM_PREFIX  := $(shell brew --prefix vasm)
  VLINK_PREFIX := $(shell brew --prefix vlink)
endif

AS := $(VASM_PREFIX)/bin/vasmm68k_mot
LD := $(VLINK_PREFIX)/bin/vlink
VBCC_LIBS ?= $(VBCC_PREFIX)/targets/m68k-amigaos/lib
STARTUP := $(VBCC_PREFIX)/targets/m68k-amigaos/lib/startup.o

ifdef VBCC_LIBS
LIBDIR += $(VBCC_LIBS)
endif

else ifdef LINUX_GCC_HOST
# ---------- m68k-amigaos-gcc (Bebbo, legacy) ----------
CC     := $(TOOLCHAIN)/bin/m68k-amigaos-gcc
GAS    := $(TOOLCHAIN)/bin/m68k-amigaos-as
AS     := $(TOOLCHAIN)/bin/vasmm68k_mot
LD     := $(TOOLCHAIN)/bin/m68k-amigaos-ld
CCOUT  := -o
STARTUP := $(TOOLCHAIN)/m68k-amigaos/lib/crt0.o

INCDIR  += $(TOOLCHAIN)/m68k-amigaos/ndk-include
LIBDIR  += $(TOOLCHAIN)/m68k-amigaos/lib
LIBDIR  += $(TOOLCHAIN)/m68k-amigaos/ndk/lib
LIBDIR  += $(TOOLCHAIN)/lib/gcc/m68k-amigaos/6.4.1b
LIBDIR  += $(TOOLCHAIN)/lib/gcc/m68k-amigaos/6.5.0b
LIBDIR  += $(TOOLCHAIN)/lib

else
# ---------- VBCC (default) ----------
ifeq ($(UNAME_S),Darwin)
  # macOS: tools from Homebrew.
  VBCC_PREFIX  := $(shell brew --prefix vbcc)
  VASM_PREFIX  := $(shell brew --prefix vasm)
  VLINK_PREFIX := $(shell brew --prefix vlink)

  CC     := $(VBCC_PREFIX)/bin/vbccm68k
  GAS    := $(VASM_PREFIX)/bin/vasmm68k_mot
  AS     := $(VASM_PREFIX)/bin/vasmm68k_mot
  LD     := $(VLINK_PREFIX)/bin/vlink

  INCDIR  += $(VBCC_PREFIX)/targets/m68k-amigaos/include
  LIBDIR  += $(VBCC_PREFIX)/targets/m68k-amigaos/lib
  STARTUP := $(VBCC_PREFIX)/targets/m68k-amigaos/lib/startup.o
else
  # Linux: tools from TOOLCHAIN directory.
  CC     := $(TOOLCHAIN)/bin-linux/vbccm68k
  GAS    := $(TOOLCHAIN)/bin-linux/vasmm68k_mot
  AS     := $(TOOLCHAIN)/bin-linux/vasmm68k_mot
  LD     := $(TOOLCHAIN)/bin-linux/vlink

  INCDIR  += $(TOOLCHAIN)/targets/m68k-amigaos/ndk/Include_H
  INCDIR  += $(TOOLCHAIN)/targets/m68k-amigaos/ndk/Include_I
  INCDIR  += $(TOOLCHAIN)/targets/m68k-amigaos/include
  LIBDIR  += $(TOOLCHAIN)/targets/m68k-amigaos/lib
  STARTUP := $(TOOLCHAIN)/targets/m68k-amigaos/lib/startup.o
endif

CCOUT := "-o="

INCDIR += $(TORNADO_BASE)/third_party/ndk/Include_H
INCDIR += $(TORNADO_BASE)/third_party/ndk/Include_I
INCDIR += $(TORNADO_BASE)/third_party/ndk/
endif

################################################################################

OBJECTS  := $(OBJECTS:%=$(BUILDDIR)/%)

################################################################################

ifdef GCC_ELF_HOST
VASM_FMT := -Felf
else
VASM_FMT := -Fhunk
endif

P61FLAGS := -quiet
P61FLAGS += $(VASM_FMT)
P61FLAGS += -phxass
P61FLAGS += -D__AMIGA__
P61FLAGS += -D__VASM__

ASFLAGS := -quiet      # Do not print the copyright notice and the final statistics.
ASFLAGS += $(VASM_FMT) # Output format: ELF for GCC_ELF_HOST, hunk otherwise.
ASFLAGS += -align      # Enables 16-bit alignment for constant declaration.
ASFLAGS += -phxass     # PhxAss-compatibilty mode.
ASFLAGS += -x          # Show error message, when referencing an undefined symbol.
ASFLAGS += -noesc      # No escape character sequence.
ASFLAGS += -nosym      # Strips all local symbols from the output file.
ASFLAGS += -m68060     # Generate code for the MC68060 CPU.
ASFLAGS += -opt-allbra # Optimize branch instructions
ASFLAGS += -opt-fconst # Floating point constants are loaded with the lowest precision possible.
ASFLAGS += -opt-lsl    # Allows optimization of LSL into ADD.
ASFLAGS += -opt-movem  # Enables optimization from MOVEM <ea>,Rn into MOVE <ea>,Rn.
ASFLAGS += -opt-mul    # Optimize multplications to shifts.
ASFLAGS += -opt-div    # Optimize divisors to shifts.
ASFLAGS += -opt-pea    # Enables optimization from MOVE #x,-(SP) into PEA x.
ASFLAGS += -opt-speed  # Optimize for speed, even if this would increase code size.
ASFLAGS += -opt-st     # Enables optimization from MOVE.B #-1,<ea> into ST <ea>.
ASFLAGS += -D__AMIGA__
ASFLAGS += -D__VASM__

ifdef GCC_ELF_HOST
# NDK assembly includes for vasm only (not added to INCDIR for C compilation).
ASFLAGS += -I$(TORNADO_BASE)/third_party/ndk/Include_H
ASFLAGS += -I$(TORNADO_BASE)/third_party/ndk/Include_I
ASFLAGS += -I$(TORNADO_BASE)/third_party/ndk/
P61FLAGS += -I$(TORNADO_BASE)/third_party/ndk/Include_H
P61FLAGS += -I$(TORNADO_BASE)/third_party/ndk/Include_I
P61FLAGS += -I$(TORNADO_BASE)/third_party/ndk/
# Assembly files with chip memory sections (data_c, bss_c) must use -Fhunk so
# that vlink preserves the MEMF_CHIP attribute in the output hunk executable.
ASFLAGS_CHIP := $(subst -Felf,-Fhunk,$(ASFLAGS))
endif


################################################################################
# Compiler flags (mutually exclusive).
################################################################################

ifdef GCC_ELF_HOST
CCFLAGS += -c                    # Compile only, do not link.
CCFLAGS += -std=c99              # C99 standard.
CCFLAGS += -O2                   # Optimise for speed. Benchmark -O3 if desired.
CCFLAGS += -m68060               # Full 68060 instruction set.
CCFLAGS += -mtune=68060          # Schedule for 68060 pipeline.
CCFLAGS += -mhard-float          # Use FPU hardware instructions.
CCFLAGS += -fleading-underscore  # Emit _symbol names to match vasm convention.
CCFLAGS += -fomit-frame-pointer  # Free up a6 for general use.
CCFLAGS += -fno-common           # Each variable gets its own section.
CCFLAGS += -mno-align-int         # Use standard m68k ABI struct layout (AmigaOS compatible).
CCFLAGS += -mbitfield            # Use bitfield instructions (68020+).
CCFLAGS += -fno-optimize-sibling-calls  # Prevent tail-call bra.l across hunks.
CCFLAGS += -Wno-int-conversion             # Amiga APIs pass pointers in ULONG tag values.
CCFLAGS += -Wno-incompatible-pointer-types # Assembly wrappers are type-agnostic.
CCFLAGS += -D__stdargs=          # Not a GCC 14 keyword; strip it.
CCFLAGS += -D__saveds=
CCFLAGS += -D__chip=
CCFLAGS += -D__interrupt=
CCFLAGS += -D__AMIGA__
CCFLAGS += -DAMIGA
CCFLAGS += -D__GCC_ELF__

# FIXME
# audio_ahi fails to compile without this, due to generating
# Error: syntax error -- statement `jsr a6@(-0x78:W)' ignored
CCFLAGS += -Wa,--register-prefix-optional

else ifdef LINUX_GCC_HOST
# enabling this needs libnix.a, which I cant get to work
#CCFLAGS += -noixemul
# -O2 causes "code reloc is out of range" when linking
# -O2 causes "error unpacking file"
CCFLAGS += -O1
GASFLAGS := -march=68040
CCFLAGS += -S
CCFLAGS += -std=c99
CCFLAGS += -march=68040
CCFLAGS += -mtune=68040
CCFLAGS += -mhard-float
CCFLAGS += -D__AMIGA__
CCFLAGS += -DAMIGA
CCFLAGS += -D__GCC__

else
# VBCC opt bits
# bit 0 - register alloc
# bit 1 - optimizer on
# bit 2 - subexpression elimination + code propagation
# bit 3 - constant propagation
# bit 4 - dead code elimination 
# bit 5 - global opt.
# bit 7 - loop invariant code motion
# bit 8 - unused object elimination (prescindible?)
# bit 10 - alias analysis
# bit 11 - loop unrolling
# bit 12 - function inlining
# bit 14 - cross module opt.
# -O=23999 -> 101110110111111 ALL OPTS
# -O=21663 -> 101010010011111 DANGER: cross seems to trigger internal compiler bugs
# -O=5279  -> 1010010011111 reg_alloc, opt_on, subexp, const, dead_code, loop_inv, alias, inline
# -O=4119  -> 1000000010111 reg_alloc, opt_on, subexp, dead_code, inline
# -O=4115  -> 1000000010011 reg_alloc, opt_on, dead_code, inline
# -O=23    -> 10111         reg_alloc, opt_on, subexp, dead_code
CCFLAGS += -quiet              # Do not print the copyright notice.
CCFLAGS += -c99                # Switch to the 1999 ISO standard for C.
CCFLAGS += -O=5279             # reg_alloc, opt_on, subexp, const, dead_code, loop_inv, alias, inline
CCFLAGS += -no-alias-opt          
CCFLAGS += -no-delayed-popping # Force to pop arguments after every function call.
CCFLAGS += -inline-size=100    # DANGER: higher than 100 seems to trigger compiler bugs
CCFLAGS += -inline-depth=10    # tentative
CCFLAGS += -cpu=68060          # Generate code for cpu 68060.
CCFLAGS += -fpu=68060          # Generate code for fpu 68060.
CCFLAGS += -no-intz            # Avoid fintrz before each float -> int.
CCFLAGS += -D__AMIGA__
CCFLAGS += -D__VBCC__
GASFLAGS := $(ASFLAGS)
endif

################################################################################
# Linker flags (mutually exclusive).
################################################################################

ifdef GCC_ELF_HOST
LDFLAGS := -bamigahunk                # Output AmigaDos hunk format from ELF objects.
LDFLAGS += -Bstatic                   # Static linking only.
LDFLAGS += -x                         # Discard local symbols.
LDFLAGS += -Cvbcc                     # VBCC style constructors (when using VBCC libs).
LDFLAGS += -nostdlib                  # Ignore default library search path.
ifdef VBCC_LIBS
LDFLAGS += -lm060                     # 68060 math library.
LDFLAGS += -lamiga                    # Amiga library stubs.
LDFLAGS += -lvc                       # VBCC C runtime library.
LDFLAGS += -lauto                     # Auto-open libraries.
endif

else ifdef LINUX_GCC_HOST
LDFLAGS := -notstdlib
LDFLAGS += -lm -lc -lstubs -lgcc -lamiga

else
LDFLAGS := -Bstatic                   # Turn of dynamic linking for all library specifiers.
LDFLAGS += -bamigahunk                # AmigaDos hunk format.
LDFLAGS += -x                         # Discard all local symbols in the input files.
LDFLAGS += -Cvbcc                     # Vbcc style constructors.
LDFLAGS += -nostdlib                  # Ignore default library search path.
LDFLAGS += -lm060                     # Include m060.lib in the output.
LDFLAGS += -lamiga                    # Include amiga.lib in the output.
LDFLAGS += -lvc                       # Include vc.lib in the output.
LDFLAGS += -lauto                     # Include auto.lib in the output.
endif


################################################################################
MKDIR   = @mkdir -p
RM      = rm -rf
QUIET	= @
ECHO	= echo 
################################################################################

all: $(TARGET)


$(TARGET): $(OBJECTS) Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(LD) -> $@"
	$(QUIET)$(LD) $(STARTUP) $(addprefix -L,$(LIBDIR)) $(OBJECTS) $(LDFLAGS) -o $@

ifdef GCC_ELF_HOST
# GCC ELF: single-step compile, C directly to ELF .o
$(BUILDDIR)/%.o: $(TORNADO_SRCDIR)/%.c Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CC) -> $@"
	$(QUIET)$(CC) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(ZINCDIR)) $(CCFLAGS) $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CC) -> $@"
	$(QUIET)$(CC) $(addprefix -I,$(INCDIR)) $(CCFLAGS) $< -o $@

else
# VBCC / old GCC: two-step compile, C -> .s then GAS -> .o
$(BUILDDIR)/%.o: $(TORNADO_SRCDIR)/%.c Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CC) -> $@"
	$(QUIET)$(CC) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(ZINCDIR)) $(CCFLAGS) $< $(CCOUT)$(@:%.o=%.s)
	$(QUIET)$(GAS) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(ZINCDIR)) $(GASFLAGS) -o $@ $(@:%.o=%.s)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CC) -> $@"
	$(QUIET)$(CC) $(addprefix -I,$(INCDIR)) $(CCFLAGS) $< $(CCOUT)$(@:%.o=%.s)
	$(QUIET)$(GAS) $(addprefix -I,$(INCDIR)) $(GASFLAGS) -o $@ $(@:%.o=%.s)

endif

# Assembly rules are shared: vasm handles both hunk and ELF via VASM_FMT.
$(BUILDDIR)/mod_replay.o: $(TORNADO_SRCDIR)/mod_replay.s Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(AS) -> $@"
	$(QUIET)$(AS) $(addprefix -I,$(INCDIR)) $(P61FLAGS) -o $@ $<

ifdef GCC_ELF_HOST
# paula_output.s uses data_c/bss_c sections for chip memory DMA buffers.
# Must be assembled as hunk format so vlink preserves MEMF_CHIP attributes.
$(BUILDDIR)/amiga/paula_output.o: $(TORNADO_SRCDIR)/amiga/paula_output.s Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(AS/CHIP) -> $@"
	$(QUIET)$(AS) $(addprefix -I,$(INCDIR)) $(ASFLAGS_CHIP) -o $@ $<
endif

$(BUILDDIR)/%.o: $(TORNADO_SRCDIR)/%.s Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(AS) -> $@"
	$(QUIET)$(AS) $(addprefix -I,$(INCDIR)) $(ASFLAGS) -o $@ $<

$(BUILDDIR)/%.o: $(SRCDIR)/%.s Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(AS) -> $@"
	$(QUIET)$(AS) $(addprefix -I,$(INCDIR)) $(ASFLAGS) -o $@ $<

clean:
	$(QUIET)$(RM) $(BUILDDIR) $(TARGET)

################################################################################
