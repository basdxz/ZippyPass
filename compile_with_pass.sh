#!/bin/bash
set -e

if [ $# -ne 1 ]; then
  echo "Usage: $0 <source.c>"
  exit 1
fi

SRC=$1
PASS_PATH="./build/src/ZippyPass.so"

# Check if pass exists
if [ ! -f "$PASS_PATH" ]; then
  echo "Error: ZippyPass not found at $PASS_PATH"
  echo "Build the project first with ./build.sh"
  exit 1
fi

# Compile and run pipeline in one go
clang -O0 -S -emit-llvm "$SRC" -o - | \
  opt -load-pass-plugin "$PASS_PATH" -passes=zippy -S | \
  clang -O3 -x ir -o "${SRC%.c}" -

echo "Compiled ${SRC} with ZippyPass to ${SRC%.c}"