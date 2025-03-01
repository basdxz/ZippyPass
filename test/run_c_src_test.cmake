# This file defines the tests for *.c source files.
#
# A unique directory is expected for each test file.
#
# An input file named `input.c` will be used to emit the initial IR `input.ll`
#
# Which will then be run through the optimisation pass to create the output `output.ll` for inspection.

# Emit initial IR
#
# EG: `clang -S -emit-llvm -O0 -x c input.c -o input.ll -fno-discard-value-names -g`
execute_process(
        COMMAND ${CLANG_EXE} -S -emit-llvm -O0
        -x c ${TEST_DIR}/input.c
        -o ${TEST_DIR}/input.ll
        -fno-discard-value-names -g
        RESULT_VARIABLE PROC_RESULT
)

# Check Result
if(NOT PROC_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to emit IR")
endif()

# Run the optimization pass
#
# EG: `opt -load-pass-plugin ZippyPass.so -passes=zippy input.ll -o output.ll -S`
execute_process(
        COMMAND ${OPT_EXE} -load-pass-plugin ${PLUGIN_PATH}
        -passes=zippy
        ${TEST_DIR}/input.ll
        -o ${TEST_DIR}/output.ll
        -S
        RESULT_VARIABLE PROC_RESULT
)

# Check Result
if(NOT PROC_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to run optimization pass")
endif()
