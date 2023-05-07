add_executable(hello_128MHz
        hello_128MHz.c
        )

# pull in common dependencies and additional clocks hardware support
target_link_libraries(hello_128MHz pico_stdlib hardware_clocks
    pico_bootsel_via_double_reset
)

pico_enable_stdio_usb(hello_128MHz 1)
pico_enable_stdio_uart(hello_128MHz 0)


# create map/bin/hex file etc.
pico_add_extra_outputs(hello_128MHz)


