#!/bin/bash
set -e

# Check deps
command -v llvm-config >/dev/null 2>&1 || { echo "Error: llvm-config not found"; exit 1; }
command -v clang >/dev/null 2>&1 || { echo "Error: clang not found"; exit 1; }
command -v cmake >/dev/null 2>&1 || { echo "Error: cmake not found"; exit 1; }
command -v ninja >/dev/null 2>&1 || { echo "Error: ninja not found"; exit 1; }

# Set env vars
export CMAKE_GENERATOR="Ninja"
export CMAKE_MAKE_PROGRAM="ninja"
export CC="clang"
export CXX="clang++"

# Build
mkdir -p build
cd build
cmake .. -G Ninja
ninja
cd ..

echo "Build completed. Run: opt -load-pass-plugin build/src/ZippyPass.so -passes=zippy input.ll -o output.ll -S"