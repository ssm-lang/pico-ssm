cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(mytest_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

add_executable(mytest)

add_subdirectory(src)

add_subdirectory(pio)

add_subdirectory(lib)

target_link_libraries(mytest
    pico_stdlib
    hardware_pio
    hardware_flash
    ssm-runtime
)

set_target_properties(pico_stdlib
  PROPERTIES
  CMAKE_EXPORT_COMPILE_COMMANDS ON
  ENABLE_EXPORTS TRUE
)

pico_enable_stdio_usb(mytest 0)
pico_enable_stdio_uart(mytest 1)
pico_enable_stdio_semihosting(mytest 0)
pico_add_extra_outputs(mytest)
# pico_set_binary_type(mytest no_flash)
# pico_set_binary_type(mytest copy_to_ram)
