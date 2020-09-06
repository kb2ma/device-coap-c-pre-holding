#!/bin/sh
set -e -x

# Find root directory and system type

ROOT=$(dirname $(dirname $(readlink -f $0)))
echo $ROOT
cd $ROOT

# Cmake debug build

mkdir -p $ROOT/build/debug
cd $ROOT/build/debug
cmake -DCMAKE_BUILD_TYPE=debug $ROOT/src/c
make 2>&1 | tee debug.log

