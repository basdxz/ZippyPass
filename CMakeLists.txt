cmake_minimum_required(VERSION 3.10)
project(Zippy)

# LLVM uses C++17
set(CMAKE_CXX_STANDARD 17)

# Include ccache if present
find_program(CCACHE_EXE ccache)
if(CCACHE_EXE)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_EXE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXE}")
    message(STATUS "Using ccache: ${CCACHE_EXE}")
endif()

# Load LLVMConfig.cmake. If this fails, consider setting `LLVM_DIR` to point
# to your LLVM installation's `lib/cmake/llvm` directory.
find_package(LLVM REQUIRED CONFIG)

# Include the part of LLVM's CMake libraries that defines
# `add_llvm_pass_plugin`.
include(AddLLVM)

# Use LLVM's preprocessor definitions, include directories, and library search
# paths.
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# Add the source directory
add_subdirectory(src)

# Add the test directory
enable_testing()
add_subdirectory(test)