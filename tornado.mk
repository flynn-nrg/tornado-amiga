#################################################################################
# Do NOT modify this makefile directly!!!
#################################################################################

#################################################################################
TORNADO_OBJ = amiga/startup.o amiga/cpu.o amiga/aga.o amiga/assets.o hardware_check.o amiga/graphics.o memory.o amiga/system.o amiga/copper.o debug.o amiga/c2p1x1_8_c5.o amiga/c2p1x1_8_c5_040_16_9.o amiga/c2p32.o amiga/c2p64.o amiga/c2p1x1_6_c5_040.o amiga/c2p1x1_8_c5_040.o amiga/c2p1x1_8_c5_040_scanline.o amiga/c2p1x1_4_c5_16_9.o amiga/audio.o amiga/paula_output.o amiga/audio_lowlevel.o wav_delta.o tndo.o amiga/splash.o amiga/c2p1x1_8_c5_bm.o amiga/mod_replay.o amiga/c2p1x1_4_c5_16_9_h.o amiga/chrono.o amiga/freq_estimation.o telemetry.o prof.o lzw_loader.o amiga/lzw_unpack.o amiga/lzw_unpack_inner.o amiga/time.o amiga/mod_replay_os_legal.o lzss_loader.o lzh_loader.o amiga/lzss_unpack.o amiga/lzh_unpack.o amiga/tndo_assert.o tndo_file.o amiga/display.o dprint.o lzw_unpack_stream.o lzss_unpack_stream.o ddpcm_loader.o ddpcm_decode.o amiga/ddpcm_lowlevel.o c2p.o

TORNADO_SRCDIR = $(TORNADO_BASE)/src
TORNADO_THIRD_PARTY_DIR = $(TORNADO_BASE)/third_party

#################################################################################
LZW_BASE = $(TORNADO_BASE)/tools/compress

DDPCM_BASE = $(TORNADO_BASE)/tools/ddpcm
DDPCM_INCDIR = $(DDPCM_BASE)

ROCKET_BASE = $(TORNADO_BASE)/third_party/rocket/lib
ROCKET_INCDIR = $(ROCKET_BASE)
#################################################################################

INCDIR = $(TORNADO_BASE)/include
ifdef LINUX_GCC_HOST
INCDIR += $(TORNADO_BASE)/include_amiga_math
endif
INCDIR += $(TORNADO_BASE)/third_party
INCDIR += $(ROCKET_INCDIR)
INCDIR += $(LZW_BASE)
INCDIR += $(DDPCM_INCDIR)
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

ifdef OSX_BREW_HOST
VBCC   := $(shell brew --prefix vbcc)
VASM   := $(shell brew --prefix vasm)
VLINK  := $(shell brew --prefix vlink)

INCDIR  += $(VBCC)/targets/m68k-amigaos/include
LIBDIR  += $(VBCC)/targets/m68k-amigaos/lib
STARTUP := $(VBCC)/targets/m68k-amigaos/lib/startup.o

CC     := $(VBCC)/bin/vbccm68k
AS     := $(VASM)/bin/vasmm68k_mot
LD     := $(VLINK)/bin/vlink
endif

ifdef OSX_HOST
VBCC_HOST = true
TOOL_BIN = bin-osx
endif 

ifdef LINUX_VBCC_HOST
VBCC_HOST = true
TOOL_BIN = bin
endif 

ifdef VBCC_HOST
INCDIR  += $(TORNADO_BASE)/third_party/ndk/ndk_3.9/include/include_h
INCDIR  += $(TORNADO_BASE)/third_party/ndk/ndk_3.9/include/include_i
INCDIR  += $(TORNADO_BASE)/third_party/ndk/

INCDIR  += $(TOOLCHAIN)/targets/m68k-amigaos/ndk/include_h
INCDIR  += $(TOOLCHAIN)/targets/m68k-amigaos/ndk/include_i
INCDIR  += $(TOOLCHAIN)/targets/m68k-amigaos/include
LIBDIR  += $(TOOLCHAIN)/targets/m68k-amigaos/lib
STARTUP := $(TOOLCHAIN)/targets/m68k-amigaos/lib/startup.o

CC     := $(TOOLCHAIN)/$(TOOL_BIN)/vbccm68k
GAS    := $(TOOLCHAIN)/$(TOOL_BIN)/vasmm68k_mot
AS     := $(TOOLCHAIN)/$(TOOL_BIN)/vasmm68k_mot
LD     := $(TOOLCHAIN)/$(TOOL_BIN)/vlink
CCOUT  := "-o="
endif

ifdef LINUX_GCC_HOST
LIBDIR  += $(TOOLCHAIN)/m68k-amigaos/lib

INCDIR  += $(TOOLCHAIN)/m68k-amigaos/ndk-include
LIBDIR  += $(TOOLCHAIN)/m68k-amigaos/ndk/lib
LIBDIR  += $(TOOLCHAIN)/lib/gcc/m68k-amigaos/6.4.1b
LIBDIR  += $(TOOLCHAIN)/lib/gcc/m68k-amigaos/6.5.0b
LIBDIR  += $(TOOLCHAIN)/lib
STARTUP := $(TOOLCHAIN)/m68k-amigaos/lib/crt0.o

CC     := $(TOOLCHAIN)/bin/m68k-amigaos-gcc
GAS    := $(TOOLCHAIN)/bin/m68k-amigaos-as
AS     := $(TOOLCHAIN)/bin/vasmm68k_mot
LD     := $(TOOLCHAIN)/bin/m68k-amigaos-ld
CCOUT  := -o
endif

################################################################################

OBJECTS  := $(OBJECTS:%=$(BUILDDIR)/%)

################################################################################

P61FLAGS := -quiet
P61FLAGS += -Fhunk
P61FLAGS += -phxass
P61FLAGS += -D__AMIGA__
P61FLAGS += -D__VASM__

ASFLAGS := -quiet      # Do not print the copyright notice and the final statistics.
ASFLAGS += -Fhunk      # Use module "hunk" as output driver.
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


################################################################################
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

# -O=23999 -> 101110110111111 reg_alloc, opt_on, subexp, const, dead_code, global, loop_inv, unused_obj, alias, unroll, inline, cross (ALL OPTS)
# -O=21663 -> 101010010011111 reg_alloc, opt_on, subexp, const, dead_code, loop_inv, alias, inline, cross (DANGER: cross seems to trigger internal compiler bugs)
# -O=5279  -> 1010010011111 reg_alloc, opt_on, subexp, const, dead_code, loop_inv, alias, inline
# -O=4119  -> 1000000010111 reg_alloc, opt_on, subexp, dead_code, inline
# -O=4115  -> 1000000010011 reg_alloc, opt_on, dead_code, inline
# -O=23    -> 10111         reg_alloc, opt_on, subexp, dead_code

ifdef VBCC_HOST
CCFLAGS += -quiet              # Do not print the copyright notice.
CCFLAGS += -c99                # Switch to the 1999 ISO standard for C.
CCFLAGS += -O=5279             # eg_alloc, opt_on, subexp, const, dead_code, loop_inv, alias, inline
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

ifdef LINUX_GCC_HOST

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
#CCFLAGS += -warn=-1
endif

################################################################################

ifdef VBCC_HOST
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

ifdef LINUX_GCC_HOST
LDFLAGS := -notstdlib
LDFLAGS += -lm -lc -lstubs -lgcc -lamiga
endif


################################################################################

MKDIR    = @mkdir -p
RM       = rm -rf

################################################################################

all: $(TARGET)


$(TARGET): $(OBJECTS) Makefile
	$(MKDIR) $(dir $@)
	$(LD) $(STARTUP) $(addprefix -L,$(LIBDIR)) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILDDIR)/%.o: $(TORNADO_SRCDIR)/%.c Makefile
	$(MKDIR) $(dir $@)
	$(CC) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(ZINCDIR)) $(CCFLAGS) $< $(CCOUT)$(@:%.o=%.s)
	$(GAS) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(ZINCDIR)) $(GASFLAGS) -o $@ $(@:%.o=%.s)

$(BUILDDIR)/mod_replay.o: $(TORNADO_SRCDIR)/mod_replay.s Makefile
	$(MKDIR) $(dir $@)
	$(AS) $(addprefix -I,$(INCDIR)) $(P61FLAGS) -o $@ $<

$(BUILDDIR)/%.o: $(TORNADO_SRCDIR)/%.s Makefile
	$(MKDIR) $(dir $@)
	$(AS) $(addprefix -I,$(INCDIR)) $(ASFLAGS) -o $@ $<

$(BUILDDIR)/%.o: $(SRCDIR)/%.c Makefile
	$(MKDIR) $(dir $@)
	$(CC) $(addprefix -I,$(INCDIR)) $(CCFLAGS) $< $(CCOUT)$(@:%.o=%.s)
	$(GAS) $(addprefix -I,$(INCDIR)) $(GASFLAGS) -o $@ $(@:%.o=%.s)

$(BUILDDIR)/%.o: $(SRCDIR)/%.s Makefile
	$(MKDIR) $(dir $@)
	$(AS) $(addprefix -I,$(INCDIR)) $(ASFLAGS) -o $@ $<

clean:
	$(RM) $(BUILDDIR) $(TARGET)

################################################################################
