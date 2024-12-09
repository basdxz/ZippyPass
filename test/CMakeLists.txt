# This file will iterate over the `test/` directory and create tests for each file present within.

# Use system clang/opt for now
find_program(CLANG_EXE clang REQUIRED)
find_program(OPT_EXE opt REQUIRED)

# Creates a test for a given `*.c` source file.
#
# The test definition can be found within `test/run_c_src_test.cmake`.
function(add_c_src_test C_SRC_TEST_INPUT)
    # Get the test name without .c extension to use as the test name.
    #
    # EG: `test_foo.c` -> `test_foo`
    get_filename_component(TEST_NAME ${C_SRC_TEST_INPUT} NAME_WE)

    # Create test work directory
    #
    # EG: `test_foo.c` -> `test/c_src/test_foo`
    set(TEST_DIR ${CMAKE_BINARY_DIR}/test/c_src/${TEST_NAME})
    file(MAKE_DIRECTORY ${TEST_DIR})

    # Copy the test input into the test work directory
    #
    # EG: test/test_foo.c -> temp/test_foo/input.c
    configure_file(${C_SRC_TEST_INPUT}
            ${TEST_DIR}/input.c
            COPYONLY)

    # Add the test into cmake with the prefix `c_src`
    #
    # EG: `test_foo.c` -> `c_src_test_foo`
    add_test(
            NAME "c_src_${TEST_NAME}"
            COMMAND ${CMAKE_COMMAND}
            -DTEST_DIR=${TEST_DIR}
            -DCLANG_EXE=${CLANG_EXE}
            -DOPT_EXE=${OPT_EXE}
            -DPLUGIN_PATH=$<TARGET_FILE:ZippyPass>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_c_src_test.cmake
    )

    # Set the test properties to depend on the root project
    # And mark the working directory
    set_tests_properties("c_src_${TEST_NAME}" PROPERTIES
            DEPENDS ZippyPass
            WORKING_DIRECTORY ${TEST_DIR}
    )
endfunction()

# Collect all files in the test root dir.
#
# The files should be present in the same directory as this CMakeLists.txt,
# but the resulting paths will be absolute.
#
# EG: `test/test_foo.c`, `test/test_bar.c`
file(GLOB C_SRC_TEST_INPUTS "*.c")

# Create a test for each .c file
foreach(C_SRC_TEST_INPUT ${C_SRC_TEST_INPUTS})
    add_c_src_test(${C_SRC_TEST_INPUT})
endforeach()