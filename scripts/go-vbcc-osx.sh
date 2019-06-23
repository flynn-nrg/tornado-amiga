#!/bin/bash
#
# Runs a bash shell with correct env setup to cross compile.

# Root of demo repository.
export DEMO=${PWD}/..

# Root of directory shared with emulator.
export SHARED=${HOME}/Documents/FS-UAE/HardDisk/shared/

# To be able to run vbcc, vlink, vasm.
export TOOLCHAIN=${DEMO}/toolchain
export PATH=${TOOLCHAIN}/bin-osx:${PATH}

export OSX_HOST=true

exec bash

