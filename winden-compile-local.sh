#!/bin/bash

set -x +

export SHARED=/home/winden/Documents/FS-UAE/HardDrives/dh1/t
export GCC_ELF_HOST=true
export PATH=/opt/m68k/output/bin:/opt/m68k/vasm-vlink-vbcc/bin:$PATH

cd $1
shift

$*
