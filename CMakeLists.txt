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
add_subdirectory(pio)

# Where we can write unit tests for
add_subdirectory(tests)

# Where our main application
add_subdirectory(src)

# Various experimental programs
add_subdirectory(serial-blink0)
add_subdirectory(serial-blink1)
add_subdirectory(serial-blink2)
add_subdirectory(pio-blink0)
