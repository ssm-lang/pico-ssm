cmake_minimum_required(VERSION 3.13)

include(cmake/ensure_libs.cmake)
include(cmake/pico_sdk_import.cmake)

project(mytest_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

# Defines the ssm-runtime library
add_subdirectory(lib)

# Defines the ssm-pio-header (header-only) and ssm-pio libraries
add_subdirectory(ssm-rp2040)

# Where we can write unit tests for
add_subdirectory(tests)

# Where we write sslang programs
add_subdirectory(programs)
