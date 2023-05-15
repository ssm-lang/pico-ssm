set(program_name interactive-pwm)
set(sslang_program pwm.ssl)

add_executable(${program_name})

target_sources(${program_name}
  # PRIVATE main.c
  # PRIVATE pico-tick.c
  PRIVATE ${sslang_program}.c
  )

target_compile_options(${program_name} PRIVATE -Wall)

pico_generate_pio_header(${program_name} ${CMAKE_CURRENT_LIST_DIR}/ssm-gpio.pio)

target_link_libraries(${program_name}
  pico_stdlib
  # Including the double_reset library makes it so that two taps on
  # the reset button emulates reset with BOOTSEL, i.e., appear as a drive
  pico_bootsel_via_double_reset
  ssm-runtime
  hardware_pio
  hardware_dma
)

pico_enable_stdio_usb(${program_name} 1)
pico_enable_stdio_uart(${program_name} 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${program_name})

add_custom_Command(
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${sslang_program}
  COMMAND sslc ${CMAKE_CURRENT_SOURCE_DIR}/${sslang_program} > ${CMAKE_CURRENT_BINARY_DIR}/${sslang_program}.c
  COMMENT "Compiling ${sslang_program} to ${sslang_program}.c with sslc"
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${sslang_program}.c
)
