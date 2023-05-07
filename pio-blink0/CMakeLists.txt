add_executable(pio-blink0)

target_sources(pio-blink0
  PRIVATE main.c
  PRIVATE pico-tick.c
  PRIVATE blink.c
  )

pico_generate_pio_header(pio-blink0 ${CMAKE_CURRENT_LIST_DIR}/ssm-output.pio)


target_link_libraries(pio-blink0
  pico_stdlib
  # Including the double_reset library makes it so that two taps on
  # the reset button emulates reset with BOOTSEL, i.e., appear as a drive
  pico_bootsel_via_double_reset
  ssm-runtime
  hardware_pio
)

pico_enable_stdio_usb(pio-blink0 1)
pico_enable_stdio_uart(pio-blink0 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(pio-blink0)

add_custom_Command(
  DEPENDS ${CMAKE_SOURCE_DIR}/pio-blink0/blink.ssl
  COMMAND sslc ${CMAKE_SOURCE_DIR}/pio-blink0/blink.ssl > ${CMAKE_CURRENT_BINARY_DIR}/blink.c
  COMMENT "Runnling sslc to generate blink.c"
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/blink.c
)
