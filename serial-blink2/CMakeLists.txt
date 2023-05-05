add_executable(serial-blink2)

target_sources(serial-blink2
  PRIVATE main.c
  PRIVATE pico-tick.c
  PRIVATE blink.c
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
  DEPENDS ${CMAKE_SOURCE_DIR}/serial-blink2/blink.ssl
  COMMAND sslc ${CMAKE_SOURCE_DIR}/serial-blink2/blink.ssl > ${CMAKE_CURRENT_BINARY_DIR}/blink.c
  COMMENT "Runnling sslc to generate blink.c"
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/blink.c
)
