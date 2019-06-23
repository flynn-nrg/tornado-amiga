#!/bin/bash
#
# Runs a bash shell with correct env setup to cross compile.

# Root of demo repository.
export DEMO=${HOME}/Amiga/ClassicWB_UAE_v28/HardDisk/Devel/amiga-demo
# Root of directory shared with emulator.
export SHARED=.

# To be able to run vbcc, vlink, vasm.
export TOOLCHAIN=${DEMO}/toolchain
export PATH=${TOOLCHAIN}/bin:${PATH}

export LINUX_HOST=true

exec bash

