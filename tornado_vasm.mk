#################################################################################
# vasm flags shared by the Amiga build (tornado.mk) and the emulated 68k build
# (m68k_emu.mk), so that both assemble the same code. Do NOT modify this
# makefile directly!!!
#
# The emulated build drops -nosym: it keeps the symbols for the crash reports.
#################################################################################

TORNADO_VASM_FLAGS := -align      # Enables 16-bit alignment for constant declaration.
TORNADO_VASM_FLAGS += -phxass     # PhxAss-compatibilty mode.
TORNADO_VASM_FLAGS += -x          # Show error message, when referencing an undefined symbol.
TORNADO_VASM_FLAGS += -noesc      # No escape character sequence.
TORNADO_VASM_FLAGS += -nosym      # Strips all local symbols from the output file.
TORNADO_VASM_FLAGS += -m68060     # Generate code for the MC68060 CPU.
TORNADO_VASM_FLAGS += -opt-allbra # Optimize branch instructions
TORNADO_VASM_FLAGS += -opt-fconst # Floating point constants are loaded with the lowest precision possible.
TORNADO_VASM_FLAGS += -opt-lsl    # Allows optimization of LSL into ADD.
TORNADO_VASM_FLAGS += -opt-movem  # Enables optimization from MOVEM <ea>,Rn into MOVE <ea>,Rn.
TORNADO_VASM_FLAGS += -opt-mul    # Optimize multplications to shifts.
TORNADO_VASM_FLAGS += -opt-div    # Optimize divisors to shifts.
TORNADO_VASM_FLAGS += -opt-pea    # Enables optimization from MOVE #x,-(SP) into PEA x.
TORNADO_VASM_FLAGS += -opt-speed  # Optimize for speed, even if this would increase code size.
TORNADO_VASM_FLAGS += -opt-st     # Enables optimization from MOVE.B #-1,<ea> into ST <ea>.
TORNADO_VASM_FLAGS += -D__AMIGA__
TORNADO_VASM_FLAGS += -D__VASM__
