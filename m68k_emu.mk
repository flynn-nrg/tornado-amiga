#################################################################################
# Emulated 68k (TORNADO_68K_EMU) build rules. Do NOT modify this makefile
# directly!!!
#
# Included by the POSIX build. It expects TORNADO_BASE, BUILDDIR, CC and
# TORNADO_SOURCE_DEPENDENCIES to be set, and defines:
#   M68K_EMU_OBJS     objects to link (paths include $(BUILDDIR))
#   M68K_EMU_INCDIR   include path for code that uses tndo_m68k.h
#
# Musashi (https://github.com/kstenerud/Musashi) is an external checkout,
# next to rocket and imgui:
#   cd $(TORNADO_SOURCE_DEPENDENCIES) && git clone https://github.com/kstenerud/Musashi
#################################################################################

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
MUSASHI_CFLAGS = -c -O2 -g -w -fno-omit-frame-pointer
MUSASHI_CFLAGS += -DMUSASHI_CNF=\"m68k_emu_conf.h\"
MUSASHI_CFLAGS += -I$(M68K_EMU_SRCDIR) -I$(MUSASHI_BUILDDIR) -I$(MUSASHI_BASE)

MUSASHI_OBJS = $(MUSASHI_BUILDDIR)/m68kcpu.o $(MUSASHI_BUILDDIR)/m68kops.o
MUSASHI_OBJS += $(MUSASHI_BUILDDIR)/m68kdasm.o $(MUSASHI_BUILDDIR)/softfloat.o
MUSASHI_OBJS += $(MUSASHI_BUILDDIR)/m68k_emu_glue.o

M68K_EMU_OBJS = $(BUILDDIR)/sdl_posix/m68k_emu.o $(MUSASHI_OBJS)

MUSASHI_DEPS = $(M68K_EMU_SRCDIR)/m68k_emu_conf.h $(MUSASHI_BUILDDIR)/m68kops.h

# m68kmake generates the opcode handlers from m68k_in.c. It is a host tool.
$(MUSASHI_BUILDDIR)/m68kmake: $(MUSASHI_BASE)/m68kmake.c
	@mkdir -p $(dir $@)
	@echo "(CC) -> $@"
	@$(CC) -O1 -w -o $@ $<

$(MUSASHI_BUILDDIR)/m68kops.c $(MUSASHI_BUILDDIR)/m68kops.h: $(MUSASHI_BUILDDIR)/m68kmake $(MUSASHI_BASE)/m68k_in.c
	@echo "(GEN) -> $(MUSASHI_BUILDDIR)/m68kops.[ch]"
	@$(MUSASHI_BUILDDIR)/m68kmake $(MUSASHI_BUILDDIR) $(MUSASHI_BASE)/m68k_in.c > /dev/null

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
$(BUILDDIR)/sdl_posix/m68k_emu.o: $(M68K_EMU_SRCDIR)/m68k_emu.c $(M68K_EMU_INCDIR)/tndo_m68k.h $(MUSASHI_DEPS)
	@mkdir -p $(dir $@)
	@echo "(CC) -> $@"
	@$(CC) $(CCFLAGS) -O2 -DMUSASHI_CNF=\"m68k_emu_conf.h\" -I$(M68K_EMU_INCDIR) -I$(M68K_EMU_SRCDIR) -I$(MUSASHI_BASE) $< -o $@
