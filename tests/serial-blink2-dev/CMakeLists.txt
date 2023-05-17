add_executable(serial-blink2)

set(ssl_name blink.ssl)
set(ssl_out ${ssl_name}.c)

target_sources(serial-blink2
  PRIVATE main.c
  PRIVATE pico-tick.c
  PRIVATE ${ssl_out}
)

target_link_libraries(serial-blink2
  pico_stdlib
  # Including the double_reset library makes it so that two taps on
  # the reset button emulates reset with BOOTSEL, i.e., appear as a drive
  pico_bootsel_via_double_reset
  ssm-runtime
)

pico_enable_stdio_usb(serial-blink2 1)
pico_enable_stdio_uart(serial-blink2 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(serial-blink2)


add_custom_Command(
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${ssl_name}
  COMMAND sslc ${CMAKE_CURRENT_SOURCE_DIR}/${ssl_name} > ${CMAKE_CURRENT_BINARY_DIR}/${ssl_out}
  COMMENT "Runnling sslc to generate blink.c"
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${ssl_out}
)
