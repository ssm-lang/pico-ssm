add_executable(serial-blink0)

target_sources(serial-blink0
  PRIVATE main.c
)

target_link_libraries(serial-blink0
  pico_stdlib
)

pico_enable_stdio_usb(serial-blink0 1)
pico_enable_stdio_uart(serial-blink0 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(serial-blink0)
