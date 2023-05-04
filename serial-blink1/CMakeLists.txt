add_executable(serial-blink1)

target_sources(serial-blink1
  PRIVATE main.c
  PRIVATE pico-tick.c
)

target_link_libraries(serial-blink1
  pico_stdlib
  ssm-runtime
)

pico_enable_stdio_usb(serial-blink1 1)
pico_enable_stdio_uart(serial-blink1 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(serial-blink1)
