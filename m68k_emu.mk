#################################################################################
# Emulated 68k (TORNADO_68K_EMU) build rules. Do NOT modify this makefile
# directly!!!
#
# Included by the POSIX build. It expects TORNADO_BASE, BUILDDIR, CC, CCFLAGS
# and TORNADO_SOURCE_DEPENDENCIES to be set. 68k modules to embed:
#   TORNADO_68K_OBJS  framework asm, relative to $(TORNADO_BASE)/src
#                     (e.g. amiga/c2p1x1_8_c5_040.o)
#   DEMO_68K_OBJS     demo asm, relative to $(M68K_DEMO_SRCDIR), which
#                     defaults to $(SRCDIR) (e.g. zoom/amiga/zoom_inner.o)
# Each one is assembled with the Amiga vasm flags into a hunk object, embedded
# into C by m68k_embed and loaded by tndo_m68k_init().
#
# Defines:
#   M68K_EMU_OBJS     objects to link (paths include $(BUILDDIR))
#   M68K_EMU_INCDIR   include path for code that uses tndo_m68k.h
#
# Musashi (https://github.com/kstenerud/Musashi) is an external checkout,
# next to rocket and imgui:
#   cd $(TORNADO_SOURCE_DEPENDENCIES) && git clone https://github.com/kstenerud/Musashi
#################################################################################

# This file only adds rules: it must not change the includer's default goal.
M68K_SAVED_DEFAULT_GOAL := $(.DEFAULT_GOAL)

MUSASHI_BASE ?= $(TORNADO_SOURCE_DEPENDENCIES)/Musashi

ifeq ($(filter clean,$(MAKECMDGOALS)),)
ifeq ($(wildcard $(MUSASHI_BASE)/m68kcpu.c),)
$(error Musashi not found in $(MUSASHI_BASE). Clone it with: git clone https://github.com/kstenerud/Musashi $(MUSASHI_BASE))
endif
endif

M68K_EMU_SRCDIR = $(TORNADO_BASE)/src/sdl_posix
M68K_EMU_INCDIR = $(TORNADO_BASE)/include
MUSASHI_BUILDDIR = $(BUILDDIR)/musashi

# Musashi is compiled without the sanitizers: it relies on signed shifts and
# type punning that UBSan would report constantly.
# -fno-common: the generated m68kops.c has large tentative definitions (the
# 512KB opcode jump table) that would become common symbols, which ld64
# aligns to 32KB and then warns "reducing alignment of section
# __DATA,__common" on arm64 macOS.
MUSASHI_CFLAGS = -c -O2 -g -w -fno-omit-frame-pointer -fno-common
MUSASHI_CFLAGS += -DMUSASHI_CNF=\"m68k_emu_conf.h\"
MUSASHI_CFLAGS += -I$(M68K_EMU_SRCDIR) -I$(MUSASHI_BUILDDIR) -I$(MUSASHI_BASE)

MUSASHI_OBJS = $(MUSASHI_BUILDDIR)/m68kcpu.o $(MUSASHI_BUILDDIR)/m68kops.o
MUSASHI_OBJS += $(MUSASHI_BUILDDIR)/m68kdasm.o $(MUSASHI_BUILDDIR)/softfloat.o
MUSASHI_OBJS += $(MUSASHI_BUILDDIR)/m68k_emu_glue.o

M68K_EMU_OBJS = $(BUILDDIR)/sdl_posix/m68k_emu.o
M68K_EMU_OBJS += $(BUILDDIR)/sdl_posix/m68k_emu_hunk.o
M68K_EMU_OBJS += $(MUSASHI_OBJS)

# 68k modules.
include $(TORNADO_BASE)/tornado_vasm.mk
M68K_VASM ?= vasmm68k_mot
M68K_DEMO_SRCDIR ?= $(SRCDIR)
# Same flags as the Amiga build, minus -nosym (symbols for crash reports).
M68K_ASFLAGS = -quiet -Fhunk $(filter-out -nosym,$(TORNADO_VASM_FLAGS))
M68K_ASFLAGS += -DTORNADO_68K_EMU
M68K_ASFLAGS += -I$(TORNADO_BASE)/include
M68K_ASFLAGS += -I$(TORNADO_BASE)/third_party/ndk/Include_I
M68K_ASFLAGS += -I$(TORNADO_BASE)/third_party/ndk
M68K_ASFLAGS += -I$(TORNADO_BASE)/third_party
M68K_ASFLAGS += -I$(TORNADO_BASE)/third_party/Cinter/player
M68K_ASFLAGS += $(addprefix -I,$(LOCAL_INCDIR))
M68K_EMBED = $(BUILDDIR)/tools/m68k_embed
M68K_TORNADO_MODULES = $(TORNADO_68K_OBJS:%=$(BUILDDIR)/m68k/tornado/%)
M68K_DEMO_MODULES = $(DEMO_68K_OBJS:%=$(BUILDDIR)/m68k/demo/%)
M68K_EMU_OBJS += $(M68K_TORNADO_MODULES) $(M68K_DEMO_MODULES)

# assemble <src.s> <module name>: .s -> .hunk -> .c -> .o
define m68k_module
	@mkdir -p $(dir $@)
	@echo "(AS/68K) -> $(@:.o=.hunk)"
	@$(M68K_VASM) $(M68K_ASFLAGS) -o $(@:.o=.hunk) $(1)
	@$(M68K_EMBED) $(@:.o=.hunk) $(@:.o=.c) $(2)
	@echo "(CC) -> $@"
	@$(CC) $(CCFLAGS) -I$(M68K_EMU_INCDIR) $(@:.o=.c) -o $@
endef

$(BUILDDIR)/m68k/tornado/%.o: $(TORNADO_BASE)/src/%.s $(M68K_EMBED)
	$(call m68k_module,$<,$*.s)

$(BUILDDIR)/m68k/demo/%.o: $(M68K_DEMO_SRCDIR)/%.s $(M68K_EMBED)
	$(call m68k_module,$<,$*.s)

$(M68K_EMBED): $(TORNADO_BASE)/tools/m68k_embed/m68k_embed.c
	@mkdir -p $(dir $@)
	@echo "(CC) -> $@"
	@$(CC) -O1 -o $@ $<

MUSASHI_DEPS = $(M68K_EMU_SRCDIR)/m68k_emu_conf.h $(MUSASHI_BUILDDIR)/m68kops.h

# m68kmake generates the opcode handlers from m68k_in.c. It is a host tool.
$(MUSASHI_BUILDDIR)/m68kmake: $(MUSASHI_BASE)/m68kmake.c
	@mkdir -p $(dir $@)
	@echo "(CC) -> $@"
	@$(CC) -O1 -w -o $@ $<

# One rule writes both files. A two-target rule would run twice under make -j.
$(MUSASHI_BUILDDIR)/m68kops.h: $(MUSASHI_BUILDDIR)/m68kmake $(MUSASHI_BASE)/m68k_in.c
	@echo "(GEN) -> $(MUSASHI_BUILDDIR)/m68kops.[ch]"
	@$(MUSASHI_BUILDDIR)/m68kmake $(MUSASHI_BUILDDIR) $(MUSASHI_BASE)/m68k_in.c > /dev/null

$(MUSASHI_BUILDDIR)/m68kops.c: $(MUSASHI_BUILDDIR)/m68kops.h
	@:

$(MUSASHI_BUILDDIR)/m68kcpu.o: $(MUSASHI_BASE)/m68kcpu.c $(MUSASHI_DEPS)
	@echo "(CC) -> $@"
	@$(CC) $(MUSASHI_CFLAGS) $< -o $@

$(MUSASHI_BUILDDIR)/m68kdasm.o: $(MUSASHI_BASE)/m68kdasm.c $(MUSASHI_DEPS)
	@echo "(CC) -> $@"
	@$(CC) $(MUSASHI_CFLAGS) $< -o $@

$(MUSASHI_BUILDDIR)/softfloat.o: $(MUSASHI_BASE)/softfloat/softfloat.c $(MUSASHI_DEPS)
	@echo "(CC) -> $@"
	@$(CC) $(MUSASHI_CFLAGS) $< -o $@

$(MUSASHI_BUILDDIR)/m68kops.o: $(MUSASHI_BUILDDIR)/m68kops.c $(MUSASHI_DEPS)
	@echo "(CC) -> $@"
	@$(CC) $(MUSASHI_CFLAGS) $< -o $@

$(MUSASHI_BUILDDIR)/m68k_emu_glue.o: $(M68K_EMU_SRCDIR)/m68k_emu_glue.c $(MUSASHI_DEPS)
	@echo "(CC) -> $@"
	@$(CC) $(MUSASHI_CFLAGS) $< -o $@

# The emulator core is built with the normal flags (sanitizers included), but
# always at -O2: its memory callbacks run on every 68k access, and at -O0 the
# emulator is ~4.5x slower. It only needs Musashi's public m68k.h.
$(BUILDDIR)/sdl_posix/m68k_emu.o: $(M68K_EMU_SRCDIR)/m68k_emu.c $(M68K_EMU_INCDIR)/tndo_m68k.h $(M68K_EMU_SRCDIR)/m68k_emu_internal.h $(MUSASHI_DEPS)
	@mkdir -p $(dir $@)
	@echo "(CC) -> $@"
	@$(CC) $(CCFLAGS) -O2 -DMUSASHI_CNF=\"m68k_emu_conf.h\" -I$(M68K_EMU_INCDIR) -I$(M68K_EMU_SRCDIR) -I$(MUSASHI_BASE) $< -o $@

$(BUILDDIR)/sdl_posix/m68k_emu_hunk.o: $(M68K_EMU_SRCDIR)/m68k_emu_hunk.c $(M68K_EMU_INCDIR)/tndo_m68k.h $(M68K_EMU_SRCDIR)/m68k_emu_internal.h
	@mkdir -p $(dir $@)
	@echo "(CC) -> $@"
	@$(CC) $(CCFLAGS) -I$(M68K_EMU_INCDIR) -I$(M68K_EMU_SRCDIR) $< -o $@

.DEFAULT_GOAL := $(M68K_SAVED_DEFAULT_GOAL)
