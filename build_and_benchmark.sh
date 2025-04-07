#!/bin/bash
# Script to build the project and run benchmarks

# Exit on any error
set -e

# Create and enter build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build the ZippyPass plugin
echo "Building ZippyPass..."
cmake --build . --target ZippyPass

# Make benchmark scripts executable
chmod +x ../benchmark/run_benchmark.sh
chmod +x ../benchmark/generate_report.py

# Build and run all benchmarks
echo "Building and running benchmarks..."
cmake --build . --target all

# Generate the benchmark report
echo "Generating benchmark report..."
cmake --build . --target benchmark_report

# Print the location of the benchmark report
echo ""
echo "Benchmark report generated at: $(pwd)/benchmark_report.md"
echo ""

# Print a summary of the benchmark results
cat benchmark_report.md | grep -A 10 "## Summary"