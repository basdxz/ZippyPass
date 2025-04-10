#!/bin/bash
# Benchmark runner for struct field reordering optimization

# Exit on error
set -e

# Global settings
LLVM_PATH=${LLVM_PATH:-"$(which clang | xargs dirname)"}
CLANG_EXE=${CLANG_EXE:-"$LLVM_PATH/clang"}
OPT_EXE=${OPT_EXE:-"$(which opt)"}
PLUGIN_PATH=${PLUGIN_PATH:-"./build/src/ZippyPass.so"}
BENCHMARK_DIR=${BENCHMARK_DIR:-"./benchmark"}
RESULTS_DIR=${RESULTS_DIR:-"./benchmark_results"}
REPORT_FILE=${REPORT_FILE:-"$RESULTS_DIR/benchmark_report.md"}

# Benchmark settings
BENCHMARK_ITERATIONS=10       # Number of times to run each benchmark
DEFAULT_INNER_ITERATIONS=100  # Default iterations for the benchmarks

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Benchmarks to run
BENCHMARKS=(
    "cache_line_benchmark"
    "padding_benchmark"
    "loop_access_benchmark"
    "array_traversal_benchmark"
    "linked_list_benchmark"
)

# Usage information
function show_usage() {
    echo "Usage: $0 [-c] [-b benchmark_name] [-a] [-i iterations]"
    echo "  -c                Clean results directory before running"
    echo "  -b benchmark_name Run only specified benchmark"
    echo "  -a                Run all benchmarks (default)"
    echo "  -i iterations     Number of inner iterations to run (default: $DEFAULT_INNER_ITERATIONS)"
    echo ""
    echo "Environment variables:"
    echo "  LLVM_PATH      Path to LLVM installation (default: derived from clang location)"
    echo "  CLANG_EXE      Path to clang executable (default: \$LLVM_PATH/clang)"
    echo "  OPT_EXE        Path to opt executable (default: \$LLVM_PATH/opt)"
    echo "  PLUGIN_PATH    Path to ZippyPass plugin (default: ./build/src/ZippyPass.so)"
    echo "  BENCHMARK_DIR  Directory containing benchmark sources (default: ./benchmark)"
    echo "  RESULTS_DIR    Directory to store results (default: ./benchmark_results)"
    echo "  REPORT_FILE    Output report file (default: ./benchmark_results/benchmark_report.md)"
}

# Clean results directory
function clean_results() {
    echo -e "${BLUE}Cleaning results directory: $RESULTS_DIR${NC}"
    rm -rf "$RESULTS_DIR"
    mkdir -p "$RESULTS_DIR"
}

# Verify prerequisites
function verify_prerequisites() {
    # Check if the ZippyPass plugin exists
    if [ ! -f "$PLUGIN_PATH" ]; then
        echo -e "${YELLOW}Error: ZippyPass plugin not found at $PLUGIN_PATH${NC}"
        echo "Please build the project first or specify the correct path with PLUGIN_PATH"
        exit 1
    fi

    # Check for clang and opt
    if [ ! -x "$CLANG_EXE" ]; then
        echo -e "${YELLOW}Error: clang not found at $CLANG_EXE${NC}"
        exit 1
    fi

    if [ ! -x "$OPT_EXE" ]; then
        echo -e "${YELLOW}Error: opt not found at $OPT_EXE${NC}"
        exit 1
    fi

    # Create results directory if it doesn't exist
    mkdir -p "$RESULTS_DIR"

    # Create benchmark directory if it doesn't exist
    mkdir -p "$BENCHMARK_DIR"
}

# Compile the benchmark
function compile_benchmark() {
    local benchmark_name="$1"
    local benchmark_src="$BENCHMARK_DIR/${benchmark_name}.c"
    local benchmark_result_dir="$RESULTS_DIR/$benchmark_name"

    # Create benchmark result directory
    mkdir -p "$benchmark_result_dir"

    # Step 1: Compile to LLVM IR without optimization
    echo "Generating LLVM IR without optimization..."
    $CLANG_EXE -S -emit-llvm -O0 \
        -x c "$benchmark_src" \
        -o "$benchmark_result_dir/original.ll" \
        -fno-discard-value-names

    # Step 2: Apply the Zippy optimization pass
    echo "Applying Zippy optimization pass..."
    $OPT_EXE -load-pass-plugin "$PLUGIN_PATH" \
        -passes=zippy \
        "$benchmark_result_dir/original.ll" \
        -o "$benchmark_result_dir/optimized.ll" \
        -S

    # Step 3: Compile original IR to executable with O3
    echo "Compiling original IR with O3 optimizations..."
    $CLANG_EXE -O3 \
        -Qunused-arguments \
        "$benchmark_result_dir/original.ll" \
        -o "$benchmark_result_dir/original"

    # Step 4: Compile optimized IR to executable with O3
    echo "Compiling optimized IR with O3 optimizations..."
    $CLANG_EXE -O3 \
        -Qunused-arguments \
        "$benchmark_result_dir/optimized.ll" \
        -o "$benchmark_result_dir/optimized"
}

# Run timing measurements for a benchmark
function run_timing() {
    local original_binary="$1"
    local optimized_binary="$2"
    local name="$3"
    local output_file="$4"
    local iterations="$5"

    echo "Running $name benchmark with $iterations iterations..."

    # Arrays to store timing results
    original_times=()
    optimized_times=()

    # Run original version
    echo "  Testing original version..."
    for i in $(seq 1 $BENCHMARK_ITERATIONS); do
        # Use time command with high precision
        start_time=$(date +%s.%N)
        $original_binary $iterations > /dev/null
        end_time=$(date +%s.%N)

        # Calculate elapsed time in milliseconds
        elapsed=$(echo "($end_time - $start_time) * 1000" | bc)
        original_times+=($elapsed)
        echo "    Run $i: ${elapsed}ms"
    done

    # Run optimized version
    echo "  Testing optimized version..."
    for i in $(seq 1 $BENCHMARK_ITERATIONS); do
        # Use time command with high precision
        start_time=$(date +%s.%N)
        $optimized_binary $iterations > /dev/null
        end_time=$(date +%s.%N)

        # Calculate elapsed time in milliseconds
        elapsed=$(echo "($end_time - $start_time) * 1000" | bc)
        optimized_times+=($elapsed)
        echo "    Run $i: ${elapsed}ms"
    done

    # Calculate statistics for original
    original_total=0
    original_min=${original_times[0]}
    original_max=${original_times[0]}

    for t in "${original_times[@]}"; do
        original_total=$(echo "$original_total + $t" | bc)
        if (( $(echo "$t < $original_min" | bc -l) )); then
            original_min=$t
        fi
        if (( $(echo "$t > $original_max" | bc -l) )); then
            original_max=$t
        fi
    done

    original_avg=$(echo "scale=2; $original_total / $BENCHMARK_ITERATIONS" | bc)

    # Calculate statistics for optimized
    optimized_total=0
    optimized_min=${optimized_times[0]}
    optimized_max=${optimized_times[0]}

    for t in "${optimized_times[@]}"; do
        optimized_total=$(echo "$optimized_total + $t" | bc)
        if (( $(echo "$t < $optimized_min" | bc -l) )); then
            optimized_min=$t
        fi
        if (( $(echo "$t > $optimized_max" | bc -l) )); then
            optimized_max=$t
        fi
    done

    optimized_avg=$(echo "scale=2; $optimized_total / $BENCHMARK_ITERATIONS" | bc)

    # Calculate improvement percentage
    if (( $(echo "$original_avg > 0" | bc -l) )); then
        improvement=$(echo "scale=2; ($original_avg - $optimized_avg) / $original_avg * 100" | bc)
    else
        improvement="0"
    fi

    # Store results in a simple text format
    cat > "$output_file" << EOF
benchmark: $name
original_avg: $original_avg
original_min: $original_min
original_max: $original_max
optimized_avg: $optimized_avg
optimized_min: $optimized_min
optimized_max: $optimized_max
improvement_percent: $improvement
iterations: $iterations
EOF

    # Print summary
    echo "  Results:"
    echo "    Original: ${original_avg}ms (min: ${original_min}ms, max: ${original_max}ms)"
    echo "    Optimized: ${optimized_avg}ms (min: ${optimized_min}ms, max: ${optimized_max}ms)"
    echo "    Improvement: ${improvement}%"
}

# Run a single benchmark
function run_benchmark() {
    local benchmark_name="$1"
    local iterations="$2"
    local benchmark_src="$BENCHMARK_DIR/${benchmark_name}.c"
    local benchmark_result_dir="$RESULTS_DIR/$benchmark_name"

    echo -e "${GREEN}Running benchmark: $benchmark_name${NC}"

    # Check if benchmark source exists
    if [ ! -f "$benchmark_src" ]; then
        echo -e "${RED}Error: Benchmark source not found: $benchmark_src${NC}"
        exit 1
    fi

    # Compile the benchmark
    compile_benchmark "$benchmark_name"

    # Run timing measurements
    run_timing \
        "$benchmark_result_dir/original" \
        "$benchmark_result_dir/optimized" \
        "$benchmark_name" \
        "$benchmark_result_dir/results.txt" \
        "$iterations"

    # Extract struct definitions for comparison
    echo "Extracting struct definitions for analysis..."
    grep -A 20 "type.*struct" "$benchmark_result_dir/original.ll" > "$benchmark_result_dir/original_structs.txt" || true
    grep -A 20 "type.*struct" "$benchmark_result_dir/optimized.ll" > "$benchmark_result_dir/optimized_structs.txt" || true

    echo -e "${GREEN}Benchmark $benchmark_name completed${NC}"
    echo ""
}

# Generate a simple markdown report from benchmark results
function generate_report() {
    echo -e "${BLUE}Generating benchmark report...${NC}"

    # Find all result files
    local result_files=$(find "$RESULTS_DIR" -name "results.txt" | sort)
    local result_count=$(echo "$result_files" | wc -l)

    if [ "$result_count" -eq 0 ]; then
        echo -e "${RED}No benchmark results found in $RESULTS_DIR${NC}"
        echo "Please run benchmarks first"
        exit 1
    fi

    # Create report header
    cat > "$REPORT_FILE" << EOF
# Struct Field Reordering Optimization Benchmark Results

Generated: $(date '+%Y-%m-%d %H:%M:%S')

## Summary

| Benchmark | Original (ms) | Optimized (ms) | Improvement |
|-----------|--------------|---------------|-------------|
EOF

    # Variables for tracking average improvement
    total_improvement=0
    improved_count=0

    # Process each result file and add to summary table
    for result_file in $result_files; do
        # Extract data from result file
        benchmark=$(grep "^benchmark:" "$result_file" | cut -d' ' -f2-)
        orig_avg=$(grep "^original_avg:" "$result_file" | cut -d' ' -f2-)
        opt_avg=$(grep "^optimized_avg:" "$result_file" | cut -d' ' -f2-)
        improvement=$(grep "^improvement_percent:" "$result_file" | cut -d' ' -f2-)

        # Add row to summary table
        echo "| $benchmark | $orig_avg | $opt_avg | $improvement% |" >> "$REPORT_FILE"

        # Update average calculation
        if (( $(echo "$improvement > 0" | bc -l) )); then
            total_improvement=$(echo "$total_improvement + $improvement" | bc)
            improved_count=$((improved_count + 1))
        fi
    done

    # Add average improvement for benchmarks that showed improvement
    if [ $improved_count -gt 0 ]; then
        avg_improvement=$(echo "scale=2; $total_improvement / $improved_count" | bc)
        echo "" >> "$REPORT_FILE"
        echo "Average improvement across $improved_count benchmarks: **${avg_improvement}%**" >> "$REPORT_FILE"
    fi

    # Add detailed results section
    echo "" >> "$REPORT_FILE"
    echo "## Detailed Results" >> "$REPORT_FILE"
    echo "" >> "$REPORT_FILE"

    # Process each result file again for detailed results
    for result_file in $result_files; do
        # Extract data from result file
        benchmark=$(grep "^benchmark:" "$result_file" | cut -d' ' -f2-)
        orig_avg=$(grep "^original_avg:" "$result_file" | cut -d' ' -f2-)
        orig_min=$(grep "^original_min:" "$result_file" | cut -d' ' -f2-)
        orig_max=$(grep "^original_max:" "$result_file" | cut -d' ' -f2-)
        opt_avg=$(grep "^optimized_avg:" "$result_file" | cut -d' ' -f2-)
        opt_min=$(grep "^optimized_min:" "$result_file" | cut -d' ' -f2-)
        opt_max=$(grep "^optimized_max:" "$result_file" | cut -d' ' -f2-)
        improvement=$(grep "^improvement_percent:" "$result_file" | cut -d' ' -f2-)
        iterations=$(grep "^iterations:" "$result_file" | cut -d' ' -f2-)

        benchmark_dir=$(dirname "$result_file")

        # Add benchmark section
        echo "### $benchmark" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        echo "- **Original**: avg=${orig_avg}ms, min=${orig_min}ms, max=${orig_max}ms (iterations=$iterations)" >> "$REPORT_FILE"
        echo "- **Optimized**: avg=${opt_avg}ms, min=${opt_min}ms, max=${opt_max}ms (iterations=$iterations)" >> "$REPORT_FILE"
        echo "- **Improvement**: $improvement%" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"

        # Add struct comparison
        if [ -f "$benchmark_dir/original_structs.txt" ] && [ -f "$benchmark_dir/optimized_structs.txt" ]; then
            echo "#### Struct Transformation" >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
            echo "Original struct:" >> "$REPORT_FILE"
            echo "\`\`\`llvm" >> "$REPORT_FILE"
            cat "$benchmark_dir/original_structs.txt" >> "$REPORT_FILE"
            echo "\`\`\`" >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
            echo "Optimized struct:" >> "$REPORT_FILE"
            echo "\`\`\`llvm" >> "$REPORT_FILE"
            cat "$benchmark_dir/optimized_structs.txt" >> "$REPORT_FILE"
            echo "\`\`\`" >> "$REPORT_FILE"
            echo "" >> "$REPORT_FILE"
        fi
    done

    echo -e "${GREEN}Benchmark report generated: $REPORT_FILE${NC}"
}

# Parse command line arguments
CLEAN=0
RUN_ALL=1
SPECIFIC_BENCHMARK=""
INNER_ITERATIONS=$DEFAULT_INNER_ITERATIONS

while getopts "hcb:ai:" opt; do
    case $opt in
        h)
            show_usage
            exit 0
            ;;
        c)
            CLEAN=1
            ;;
        b)
            RUN_ALL=0
            SPECIFIC_BENCHMARK="$OPTARG"
            ;;
        a)
            RUN_ALL=1
            ;;
        i)
            INNER_ITERATIONS="$OPTARG"
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            show_usage
            exit 1
            ;;
    esac
done

# Main execution
verify_prerequisites

if [ $CLEAN -eq 1 ]; then
    clean_results
fi

# Check if benchmarks exist
for benchmark in "${BENCHMARKS[@]}"; do
    if [ ! -f "$BENCHMARK_DIR/${benchmark}.c" ]; then
        echo -e "${YELLOW}Warning: Benchmark source not found: $BENCHMARK_DIR/${benchmark}.c${NC}"
        echo "You may need to copy benchmark files to $BENCHMARK_DIR first."
    fi
done

if [ $RUN_ALL -eq 1 ]; then
    for benchmark in "${BENCHMARKS[@]}"; do
        if [ -f "$BENCHMARK_DIR/${benchmark}.c" ]; then
            run_benchmark "$benchmark" "$INNER_ITERATIONS"
        else
            echo -e "${YELLOW}Skipping missing benchmark: $benchmark${NC}"
        fi
    done
else
    if [ -f "$BENCHMARK_DIR/${SPECIFIC_BENCHMARK}.c" ]; then
        run_benchmark "$SPECIFIC_BENCHMARK" "$INNER_ITERATIONS"
    else
        echo -e "${RED}Error: Benchmark $SPECIFIC_BENCHMARK not found${NC}"
        exit 1
    fi
fi

generate_report

exit 0