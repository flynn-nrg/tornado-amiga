#################################################################################
# Headless tests for the emulated 68k core.
#
#   make            build and run the tests
#   make verbose    same, printing the crash report of every fault test
#
# Musashi is looked up in $(TORNADO_SOURCE_DEPENDENCIES)/Musashi, like the
# demo builds look up rocket and imgui.
#################################################################################

TORNADO_BASE = ../..
TORNADO_SOURCE_DEPENDENCIES ?= $(TORNADO_BASE)/..
BUILDDIR ?= build

CC := cc
AS := vasmm68k_mot

CCFLAGS = -c --std=c99 -O0 -g -fno-omit-frame-pointer -Wall -Wfatal-errors
CCFLAGS += -fsanitize=address -fsanitize=undefined
LDFLAGS = -fsanitize=address -fsanitize=undefined

# asm/: position independent routines, flat binaries loaded with
# tndo_m68k_load_code(). Same CPU and syntax flags as tornado.mk.
ASFLAGS = -quiet -Fbin -align -phxass -x -noesc -m68060

# embed/ and every framework asm file in src/amiga/ are embedded into the
# runner exactly like a demo build does it, so each tndo_m68k_init() loads
# them all.
M68K_DEMO_SRCDIR = embed
DEMO_68K_OBJS = emb_lib.o emb_main.o
TORNADO_68K_OBJS = $(patsubst $(TORNADO_BASE)/src/%.s,%.o,$(wildcard $(TORNADO_BASE)/src/amiga/*.s))

all: test

include $(TORNADO_BASE)/m68k_emu.mk

TESTS = $(basename $(notdir $(wildcard asm/*.s)))
BINS = $(TESTS:%=$(BUILDDIR)/bin/%.bin)
# hunk/: hunk objects loaded at runtime with tndo_m68k_load_hunk().
HUNK_TESTS = $(basename $(notdir $(wildcard hunk/*.s)))
HUNKS = $(HUNK_TESTS:%=$(BUILDDIR)/bin/%.hunk)
RUNNER = $(BUILDDIR)/test_m68k_emu

test: $(RUNNER) $(BINS) $(HUNKS)
	$(RUNNER) $(BUILDDIR)/bin

verbose: $(RUNNER) $(BINS) $(HUNKS)
	$(RUNNER) $(BUILDDIR)/bin -v

$(RUNNER): $(BUILDDIR)/test_m68k_emu.o $(M68K_EMU_OBJS)
	@echo "(LD) -> $@"
	@$(CC) $^ $(LDFLAGS) -o $@

$(BUILDDIR)/test_m68k_emu.o: test_m68k_emu.c $(M68K_EMU_INCDIR)/tndo_m68k.h
	@mkdir -p $(dir $@)
	@echo "(CC) -> $@"
	@$(CC) $(CCFLAGS) -I$(M68K_EMU_INCDIR) $< -o $@

$(BUILDDIR)/bin/%.bin: asm/%.s
	@mkdir -p $(dir $@)
	@echo "(AS) -> $@"
	@$(AS) $(ASFLAGS) -o $@ $<

$(BUILDDIR)/bin/%.hunk: hunk/%.s
	@mkdir -p $(dir $@)
	@echo "(AS) -> $@"
	@$(M68K_VASM) $(M68K_ASFLAGS) -o $@ $<

clean:
	rm -rf $(BUILDDIR)

.PHONY: all test verbose clean
