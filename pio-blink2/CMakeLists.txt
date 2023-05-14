add_executable(pio-blink2)

target_sources(pio-blink2
  PRIVATE main.c
  PRIVATE pico-tick.c
  PRIVATE blink.c
  )

target_compile_options(pio-blink2 PRIVATE -Wall)

pico_generate_pio_header(pio-blink2 ${CMAKE_CURRENT_LIST_DIR}/ssm-gpio.pio)


target_link_libraries(pio-blink2
  pico_stdlib
  # Including the double_reset library makes it so that two taps on
  # the reset button emulates reset with BOOTSEL, i.e., appear as a drive
  pico_bootsel_via_double_reset
  ssm-runtime
  hardware_pio
  hardware_dma
)

pico_enable_stdio_usb(pio-blink2 1)
pico_enable_stdio_uart(pio-blink2 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(pio-blink2)

add_custom_Command(
  DEPENDS ${CMAKE_SOURCE_DIR}/pio-blink2/blink.ssl
  COMMAND sslc ${CMAKE_SOURCE_DIR}/pio-blink2/blink.ssl > ${CMAKE_CURRENT_BINARY_DIR}/blink.c
  COMMENT "Compiling blink.ssl to blink.c with sslc"
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/blink.c
)
