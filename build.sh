#!/bin/bash

set -xe
CC="${CXX:-cc}"
CFLAGS="-O3 -Wall -Wextra -Wpedantic -Wconversion -std=c99 -g -DTEST_FEATURE -DTEST_SPEED"
SRC="./test.c"

$CC $CFLAGS $SRC -o test_compressor
./test_compressor
