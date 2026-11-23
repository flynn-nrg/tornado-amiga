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

# Same CPU and syntax flags as tornado.mk, flat binary output.
ASFLAGS = -quiet -Fbin -align -phxass -x -noesc -m68060

all: test

include $(TORNADO_BASE)/m68k_emu.mk

TESTS = $(basename $(notdir $(wildcard asm/*.s)))
BINS = $(TESTS:%=$(BUILDDIR)/bin/%.bin)
RUNNER = $(BUILDDIR)/test_m68k_emu

test: $(RUNNER) $(BINS)
	$(RUNNER) $(BUILDDIR)/bin

verbose: $(RUNNER) $(BINS)
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

clean:
	rm -rf $(BUILDDIR)

.PHONY: all test verbose clean
