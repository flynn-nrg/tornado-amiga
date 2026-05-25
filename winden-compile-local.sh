#!/bin/bash

set -x +

if [ $# -lt 2 ]; then
  echo "usage:   ./winden-compile-local.sh <root directory> <command to run>"
  echo "example: ./winden-compile-local.sh examples/simple_screen make clean all"
  exit 1
fi

export SHARED=/home/winden/Documents/FS-UAE/HardDrives/dh1/t

source scripts/linux-use-prebuilt-toolchain.sh

cd $1
shift

$*
