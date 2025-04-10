#!/bin/bash

# Exit on error
set -e

# Global settings (hardcoded)
PLUGIN_PATH="./build/src/ZippyPass.so"
BENCHMARK_DIR="./benchmark"
RESULTS_DIR="$BENCHMARK_DIR/results"
BENCHMARK_ITERATIONS=10
INNER_ITERATIONS=100

# Find all benchmark files in the directory
BENCHMARKS=($(find "$BENCHMARK_DIR" -name "*.c" -exec basename {} \; | sed 's/\.c$//'))

if [ ${#BENCHMARKS[@]} -eq 0 ]; then
    echo "Error: No benchmark files found in $BENCHMARK_DIR"
    exit 1
fi

# Clean results directory
rm -rf "$RESULTS_DIR"
mkdir -p "$RESULTS_DIR"

# Verify prerequisites
if [ ! -f "$PLUGIN_PATH" ]; then
    echo "Error: ZippyPass plugin not found at $PLUGIN_PATH"
    echo "Please build the project first."
    exit 1
fi

if ! which clang > /dev/null || ! which opt > /dev/null; then
    echo "Error: clang or opt not found in PATH"
    exit 1
fi

# Compile and run a single benchmark
function run_benchmark() {
    local benchmark_name="$1"
    local benchmark_src="$BENCHMARK_DIR/${benchmark_name}.c"
    local benchmark_result_dir="$RESULTS_DIR/$benchmark_name"

    echo "Running benchmark: $benchmark_name"

    # Create result directory
    mkdir -p "$benchmark_result_dir"

    # Step 1: Compile to LLVM IR without optimization
    echo "Generating LLVM IR..."
    clang -S -emit-llvm -O0 -x c "$benchmark_src" -o "$benchmark_result_dir/input.ll" -fno-discard-value-names

    # Step 2: Apply Zippy optimization pass
    echo "Applying Zippy optimization pass..."
    opt -load-pass-plugin "$PLUGIN_PATH" -passes=zippy "$benchmark_result_dir/input.ll" -o "$benchmark_result_dir/output.ll" -S 2> >(tee "$benchmark_result_dir/debug.txt")

    # Step 3: Compile both versions with O3
    echo "Compiling with O3 optimizations..."
    clang -O3 -Qunused-arguments "$benchmark_result_dir/input.ll" -o "$benchmark_result_dir/input"
    clang -O3 -Qunused-arguments "$benchmark_result_dir/output.ll" -o "$benchmark_result_dir/output"

    # Step 4: Run timing measurements
    echo "Running performance tests..."

    # Arrays for results
    input_times=()
    output_times=()

    # Run input version
    for i in $(seq 1 $BENCHMARK_ITERATIONS); do
        start_time=$(date +%s.%N)
        "$benchmark_result_dir"/input $INNER_ITERATIONS > /dev/null
        end_time=$(date +%s.%N)

        elapsed=$(echo "($end_time - $start_time) * 1000" | bc)
        input_times+=("$elapsed")
        echo "  Input run $i: ${elapsed}ms"
    done

    # Run output version
    for i in $(seq 1 $BENCHMARK_ITERATIONS); do
        start_time=$(date +%s.%N)
        "$benchmark_result_dir"/output $INNER_ITERATIONS > /dev/null
        end_time=$(date +%s.%N)

        elapsed=$(echo "($end_time - $start_time) * 1000" | bc)
        output_times+=("$elapsed")
        echo "  Output run $i: ${elapsed}ms"
    done

    # Store raw results in JSON format
    echo "{" > "$benchmark_result_dir/results.json"
    echo "  \"benchmark\": \"$benchmark_name\"," >> "$benchmark_result_dir/results.json"
    echo "  \"iterations\": $INNER_ITERATIONS," >> "$benchmark_result_dir/results.json"
    echo "  \"input_times\": [" >> "$benchmark_result_dir/results.json"

    # Add input times to JSON
    for i in "${!input_times[@]}"; do
        if [ $i -eq $(( ${#input_times[@]} - 1 )) ]; then
            echo "    ${input_times[$i]}" >> "$benchmark_result_dir/results.json"
        else
            echo "    ${input_times[$i]}," >> "$benchmark_result_dir/results.json"
        fi
    done

    echo "  ]," >> "$benchmark_result_dir/results.json"
    echo "  \"output_times\": [" >> "$benchmark_result_dir/results.json"

    # Add output times to JSON
    for i in "${!output_times[@]}"; do
        if [ $i -eq $(( ${#output_times[@]} - 1 )) ]; then
            echo "    ${output_times[$i]}" >> "$benchmark_result_dir/results.json"
        else
            echo "    ${output_times[$i]}," >> "$benchmark_result_dir/results.json"
        fi
    done

    echo "  ]" >> "$benchmark_result_dir/results.json"
    echo "}" >> "$benchmark_result_dir/results.json"

    # Quick feedback for user without calculating averages
    echo "Results stored in $benchmark_result_dir/results.json"
    echo ""
}

# Combine all result files into a single JSON
function combine_results() {
    echo "Combining benchmark results..."

    # First create a temporary JSON with basic structure
    echo "{" > "$RESULTS_DIR/temp_results.json"
    echo "  \"benchmarks\": {" >> "$RESULTS_DIR/temp_results.json"

    # Track if we need a comma
    first=true

    for benchmark in "${BENCHMARKS[@]}"; do
        result_file="$RESULTS_DIR/$benchmark/results.json"
        if [ -f "$result_file" ]; then
            if [ "$first" = true ]; then
                first=false
            else
                echo "," >> "$RESULTS_DIR/temp_results.json"
            fi

            # Extract benchmark content without outer braces
            content=$(sed -e '1d' -e '$d' "$result_file")

            # Add benchmark name as key
            echo "    \"$benchmark\": {" >> "$RESULTS_DIR/temp_results.json"
            echo "$content" >> "$RESULTS_DIR/temp_results.json"
            echo -n "    }" >> "$RESULTS_DIR/temp_results.json"
        fi
    done

    echo "" >> "$RESULTS_DIR/temp_results.json"
    echo "  }" >> "$RESULTS_DIR/temp_results.json"
    echo "}" >> "$RESULTS_DIR/temp_results.json"

    # Check if jq is available to format the JSON
    if command -v jq &> /dev/null; then
        jq '.' "$RESULTS_DIR/temp_results.json" > "$RESULTS_DIR/full_results.json"
        rm "$RESULTS_DIR/temp_results.json"
    else
        mv "$RESULTS_DIR/temp_results.json" "$RESULTS_DIR/full_results.json"
    fi
    echo "Combined results formatted and stored in $RESULTS_DIR/full_results.json"
}

# Main execution
echo "Starting Zippy benchmarks with $INNER_ITERATIONS iterations per test"

# Run all benchmarks
for benchmark in "${BENCHMARKS[@]}"; do
    if [ -f "$BENCHMARK_DIR/${benchmark}.c" ]; then
        run_benchmark "$benchmark"
    else
        echo "Skipping missing benchmark: $benchmark"
    fi
done

combine_results

echo "All benchmarks completed!"