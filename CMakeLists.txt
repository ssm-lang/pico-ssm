cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(mytest_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

add_executable(test src/main.c)

target_link_libraries(
    test
    pico_stdlib
    hardware_flash
)

set_target_properties(pico_stdlib
  PROPERTIES
  CMAKE_EXPORT_COMPILE_COMMANDS ON
  ENABLE_EXPORTS TRUE
)

pico_enable_stdio_usb(test 0)
pico_enable_stdio_uart(test 1)
pico_enable_stdio_semihosting(test 0)
pico_add_extra_outputs(test)
# pico_set_binary_type(test no_flash)
# pico_set_binary_type(test copy_to_ram)
