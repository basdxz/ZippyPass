# This file defines the tests for *.c source files.
#
# A unique directory is expected for each test file.
#
# An input file named `input.c` will be used to emit the initial IR `input.ll`
#
# Which will then be run through the optimisation pass to create the output `output.ll` for inspection.
#
# Then, both the `input.ll` and `output.ll` are compiled into the executables `input` and `output`

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

# Compile the input IR into an executable
#
# EG: `clang input.ll -o input`
execute_process(
        COMMAND ${CLANG_EXE}
        -Qunused-arguments # Here to silence NixOS Noise
        -O3
        ${TEST_DIR}/input.ll
        -o ${TEST_DIR}/input
        RESULT_VARIABLE PROC_RESULT
)

# Check Result
if(NOT PROC_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to compile input IR")
endif()

# Compile the output IR into an executable
#
# EG: `clang output.ll -o output`
execute_process(
        COMMAND ${CLANG_EXE}
        -Qunused-arguments # Here to silence NixOS Noise
        -O3
        ${TEST_DIR}/output.ll
        -o ${TEST_DIR}/output
        RESULT_VARIABLE PROC_RESULT
)

# Check Result
if(NOT PROC_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to compile output IR")
endif()

# Runs the input executable
execute_process(
        COMMAND ${TEST_DIR}/input
        RESULT_VARIABLE PROC_RESULT
)

# Runs the output executable
execute_process(
        COMMAND ${TEST_DIR}/output
        RESULT_VARIABLE PROC_RESULT_2
)

# Compares the results
if(NOT PROC_RESULT_2 EQUAL PROC_RESULT)
    message(FATAL_ERROR "Output changed")
endif()