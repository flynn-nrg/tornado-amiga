#!/bin/bash
#
# Runs a bash shell with correct env setup to cross compile using VBCC.
#
# The host OS is auto-detected by tornado.mk (via uname).
# VBCC/VASM/VLINK are resolved from Homebrew automatically.

# Root of demo repository.
export DEMO=${PWD}/..

# Root of directory shared with emulator.
export SHARED=${HOME}/Documents/FS-UAE/HardDisk/shared/

# Verify Homebrew vbcc, vasm, and vlink are installed.
for pkg in vbcc vasm vlink; do
    if ! brew --prefix "$pkg" &> /dev/null; then
        echo "ERROR: $pkg not found via Homebrew. Install with: brew install $pkg"
        exit 1
    fi
done

exec bash
