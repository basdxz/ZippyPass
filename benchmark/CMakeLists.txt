# This file sets up benchmarks to measure the performance impact of struct field reordering
# It creates binaries from each .c file both with and without the optimization applied
# Then captures runtime metrics using an external measurement script

# Create a benchmark from a C source file with and without optimization
function(add_field_reordering_benchmark BENCHMARK_SRC)
    # Get the benchmark name without extension to use for target names
    get_filename_component(BENCHMARK_NAME ${BENCHMARK_SRC} NAME_WE)

    # Create benchmark work directory
    set(BENCHMARK_DIR ${CMAKE_BINARY_DIR}/benchmark/${BENCHMARK_NAME})
    file(MAKE_DIRECTORY ${BENCHMARK_DIR})

    # Copy the benchmark source to the work directory
    configure_file(${BENCHMARK_SRC}
            ${BENCHMARK_DIR}/source.c
            COPYONLY)

    # Step 1: Compile to LLVM IR without optimization
    add_custom_command(
            OUTPUT ${BENCHMARK_DIR}/original.ll
            COMMAND ${CLANG_EXE} -S -emit-llvm -O0
            -x c ${BENCHMARK_DIR}/source.c
            -o ${BENCHMARK_DIR}/original.ll
            -fno-discard-value-names
            DEPENDS ${BENCHMARK_DIR}/source.c
            COMMENT "Generating LLVM IR for ${BENCHMARK_NAME} (original)"
    )

    # Step 2: Compile to LLVM IR with the Zippy optimization pass
    add_custom_command(
            OUTPUT ${BENCHMARK_DIR}/optimized.ll
            COMMAND ${OPT_EXE} -load-pass-plugin $<TARGET_FILE:ZippyPass>
            -passes=zippy
            ${BENCHMARK_DIR}/original.ll
            -o ${BENCHMARK_DIR}/optimized.ll
            -S
            DEPENDS ${BENCHMARK_DIR}/original.ll ZippyPass
            COMMENT "Applying struct field reordering to ${BENCHMARK_NAME}"
    )

    # Step 3: Compile original IR to executable with standard optimizations
    add_custom_command(
            OUTPUT ${BENCHMARK_DIR}/original
            COMMAND ${CLANG_EXE} -O3
            ${BENCHMARK_DIR}/original.ll
            -o ${BENCHMARK_DIR}/original
            DEPENDS ${BENCHMARK_DIR}/original.ll
            COMMENT "Compiling original ${BENCHMARK_NAME} with O3 optimizations"
    )

    # Step 4: Compile optimized IR to executable with standard optimizations
    add_custom_command(
            OUTPUT ${BENCHMARK_DIR}/optimized
            COMMAND ${CLANG_EXE} -O3
            ${BENCHMARK_DIR}/optimized.ll
            -o ${BENCHMARK_DIR}/optimized
            DEPENDS ${BENCHMARK_DIR}/optimized.ll
            COMMENT "Compiling optimized ${BENCHMARK_NAME} with O3 optimizations"
    )

    # Step 5: Run the benchmark and measure performance
    add_custom_command(
            OUTPUT ${BENCHMARK_DIR}/results.json
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_benchmark.sh
            ${BENCHMARK_DIR}/original
            ${BENCHMARK_DIR}/optimized
            ${BENCHMARK_DIR}/results.json
            ${BENCHMARK_NAME}
            DEPENDS ${BENCHMARK_DIR}/original ${BENCHMARK_DIR}/optimized
            COMMENT "Running benchmark for ${BENCHMARK_NAME}"
    )

    # Create a custom target for this benchmark
    add_custom_target(benchmark_${BENCHMARK_NAME} ALL
            DEPENDS ${BENCHMARK_DIR}/results.json
    )

    # Register as a test
    add_test(
            NAME benchmark_${BENCHMARK_NAME}
            COMMAND ${CMAKE_COMMAND} -E cat ${BENCHMARK_DIR}/results.json
            WORKING_DIRECTORY ${BENCHMARK_DIR}
    )

    # Add to the list of benchmarks
    set_property(GLOBAL APPEND PROPERTY BENCHMARK_RESULT_FILES "${BENCHMARK_DIR}/results.json")
endfunction()

# Create a target to aggregate all benchmark results
add_custom_target(benchmark_report
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/generate_report.py ${CMAKE_BINARY_DIR}/benchmark_report.md
        COMMENT "Generating benchmark report"
)

# Use glob to find all benchmark files
file(GLOB BENCHMARK_SRCS "*.c")

# Create a benchmark for each source file
foreach(BENCHMARK_SRC ${BENCHMARK_SRCS})
    add_field_reordering_benchmark(${BENCHMARK_SRC})
endforeach()