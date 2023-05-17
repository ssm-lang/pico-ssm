add_executable(pio-blink1)

set(ssl_name blink.ssl)
set(ssl_out ${ssl_name}.c)

target_sources(pio-blink1
  PRIVATE main.c
  PRIVATE pico-tick.c
  PRIVATE ${ssl_out}
)

target_compile_options(pio-blink1 PRIVATE -Wall)

pico_generate_pio_header(pio-blink1 ${CMAKE_CURRENT_LIST_DIR}/ssm-output.pio)
pico_generate_pio_header(pio-blink1 ${CMAKE_CURRENT_LIST_DIR}/ssm-input.pio)


target_link_libraries(pio-blink1
  pico_stdlib
  # Including the double_reset library makes it so that two taps on
  # the reset button emulates reset with BOOTSEL, i.e., appear as a drive
  pico_bootsel_via_double_reset
  ssm-runtime
  hardware_pio
  hardware_dma
)

pico_enable_stdio_usb(pio-blink1 1)
pico_enable_stdio_uart(pio-blink1 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(pio-blink1)

add_custom_Command(
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${ssl_name}
  COMMAND sslc ${CMAKE_CURRENT_SOURCE_DIR}/${ssl_name} > ${CMAKE_CURRENT_BINARY_DIR}/${ssl_out}
  COMMENT "Runnling sslc to generate blink.c"
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${ssl_out}
)
