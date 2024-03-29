# This CMakeLists.txt scans the directory for sslang source files (*.ssl), and
# builds an executable with each of them, linked against the ssm-rp2040 runtime.
#
# To add a new sslang program, just add another <name>.ssl file in the pwd.
# The executable will be built to build/programs/<name>.elf.

file(GLOB programs *.ssl)

foreach(program_path ${programs})
    get_filename_component(program ${program_path} NAME_WLE)
    set(program_ssl ${program}.ssl)
    set(program_c ${program_ssl}.c)

    add_executable(${program})
    target_sources(${program} PRIVATE ${program_c})
    target_compile_options(${program} PRIVATE -Wall -Wno-parentheses)

    target_link_libraries(${program}
      pico_stdlib
      # Including the double_reset library makes it so that two taps on
      # the reset button emulates reset with BOOTSEL, i.e., appear as a drive
      pico_bootsel_via_double_reset
      ssm-rp2040
      hardware_pio
      hardware_dma
    )

    pico_enable_stdio_usb(${program} 0)
    pico_enable_stdio_uart(${program} 1)
    pico_add_extra_outputs(${program})

    add_custom_Command(
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${program_ssl}
      COMMAND sslc ${CMAKE_CURRENT_SOURCE_DIR}/${program_ssl} > ${CMAKE_CURRENT_BINARY_DIR}/${program_c}
      COMMENT "Compiling ${program_ssl} to ${program_c} with sslc"
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${program_c}
    )
endforeach()
