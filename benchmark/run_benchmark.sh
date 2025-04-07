#!/bin/bash
# Script to run benchmarks and measure performance
# Usage: ./run_benchmark.sh <original_binary> <optimized_binary> <output_json> <benchmark_name>

ORIGINAL_BINARY="$1"
OPTIMIZED_BINARY="$2"
OUTPUT_JSON="$3"
BENCHMARK_NAME="$4"

# Check if both binaries exist
if [ ! -f "$ORIGINAL_BINARY" ] || [ ! -f "$OPTIMIZED_BINARY" ]; then
    echo "Error: One or both binaries don't exist"
    exit 1
fi

# Directory containing input files
BENCHMARK_DIR=$(dirname "$ORIGINAL_BINARY")
ORIGINAL_IR="${BENCHMARK_DIR}/original.ll"
OPTIMIZED_IR="${BENCHMARK_DIR}/optimized.ll"

# Verify that the optimization was actually applied
if [ -f "$ORIGINAL_IR" ] && [ -f "$OPTIMIZED_IR" ]; then
    echo "Verifying optimization for $BENCHMARK_NAME..."

    # Check if files are different (optimization applied)
    if cmp -s "$ORIGINAL_IR" "$OPTIMIZED_IR"; then
        echo "WARNING: Original and optimized IR files are identical! Optimization may not have been applied."
        echo "Creating comparison file for debugging..."
        diff -u "$ORIGINAL_IR" "$OPTIMIZED_IR" > "${BENCHMARK_DIR}/ir_diff.txt" || true
        echo "Full diff saved to ${BENCHMARK_DIR}/ir_diff.txt"
    else
        echo "Optimization verified - IR files are different."
        # Get struct definitions from both files to verify reordering
        grep -A 10 "struct" "$ORIGINAL_IR" > "${BENCHMARK_DIR}/original_structs.txt" || true
        grep -A 10 "struct" "$OPTIMIZED_IR" > "${BENCHMARK_DIR}/optimized_structs.txt" || true
        echo "Struct definitions extracted for comparison."
    fi
fi

ITERATIONS=10  # Number of times to run each benchmark for more accurate results

function run_timings() {
    binary=$1
    name=$2

    echo "Running $name benchmark..." >&2

    times=()
    for i in $(seq 1 $ITERATIONS); do
        # Use time command with high precision
        start_time=$(date +%s.%N)
        $binary > /dev/null
        end_time=$(date +%s.%N)

        # Calculate elapsed time in milliseconds
        elapsed=$(echo "($end_time - $start_time) * 1000" | bc)
        times+=($elapsed)

        echo "  Run $i: ${elapsed}ms" >&2
    done

    # Calculate statistics
    total=0
    for t in "${times[@]}"; do
        total=$(echo "$total + $t" | bc)
    done

    avg=$(echo "scale=2; $total / $ITERATIONS" | bc)

    # Calculate min and max
    min=${times[0]}
    max=${times[0]}

    for t in "${times[@]}"; do
        if (( $(echo "$t < $min" | bc -l) )); then
            min=$t
        fi
        if (( $(echo "$t > $max" | bc -l) )); then
            max=$t
        fi
    done

    # Return as an object in stdout - without any other text
    echo "{\"avg\": $avg, \"min\": $min, \"max\": $max, \"iterations\": $ITERATIONS}"
}

# Run both binaries and collect timings
echo "Starting benchmark: $BENCHMARK_NAME" >&2
original_stats=$(run_timings "$ORIGINAL_BINARY" "Original")
optimized_stats=$(run_timings "$OPTIMIZED_BINARY" "Optimized")

# Parse the JSON objects
original_avg=$(echo $original_stats | grep -o '"avg": [0-9.]*' | cut -d' ' -f2)
optimized_avg=$(echo $optimized_stats | grep -o '"avg": [0-9.]*' | cut -d' ' -f2)

# Calculate improvement percentage
if (( $(echo "$original_avg > 0" | bc -l) )); then
    improvement=$(echo "scale=2; ($original_avg - $optimized_avg) / $original_avg * 100" | bc)
else
    improvement="0"
fi

# Create the output JSON
cat > "$OUTPUT_JSON" << EOF
{
  "benchmark": "$BENCHMARK_NAME",
  "original": $original_stats,
  "optimized": $optimized_stats,
  "improvement_percent": $improvement
}
EOF

# Print summary to stderr (not in JSON)
echo "Benchmark completed: $BENCHMARK_NAME" >&2
echo "Original: ${original_avg}ms, Optimized: ${optimized_avg}ms" >&2
echo "Improvement: ${improvement}%" >&2

exit 0