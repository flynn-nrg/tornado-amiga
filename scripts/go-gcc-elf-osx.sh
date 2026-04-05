#!/bin/bash
#
# Runs a bash shell with correct env setup to cross compile using
# m68k-amiga-elf-gcc (GCC 14) from the Amiga Debug extension.
#
# The host OS is auto-detected by tornado.mk (via uname).
# VASM/VLINK are resolved from Homebrew automatically.

# Root of demo repository.
export DEMO=${PWD}/..

# Root of directory shared with emulator.
export SHARED=${HOME}/Documents/FS-UAE/HardDisk/shared/

# The m68k-amiga-elf-gcc toolchain is provided by the vscode-amiga-debug
# extension and should already be in PATH. Verify it exists.
if ! command -v m68k-amiga-elf-gcc &> /dev/null; then
    echo "ERROR: m68k-amiga-elf-gcc not found in PATH."
    echo "Install the Amiga Debug extension or add the toolchain to PATH."
    exit 1
fi

# Verify Homebrew vasm and vlink are installed (needed for assembly + linking).
for pkg in vasm vlink vbcc; do
    if ! brew --prefix "$pkg" &> /dev/null; then
        echo "ERROR: $pkg not found via Homebrew. Install with: brew install $pkg"
        exit 1
    fi
done

export GCC_ELF_HOST=true

exec bash
