add_executable(gpio_output_sanity_test)

target_sources(gpio_output_sanity_test
    PRIVATE gpio_output_sanity_test.c
)

target_link_libraries(gpio_output_sanity_test
    pico_runtime
    pico_time
    hardware_gpio
)

pico_enable_stdio_usb(gpio_output_sanity_test 0)
pico_enable_stdio_uart(gpio_output_sanity_test 0)
pico_enable_stdio_semihosting(gpio_output_sanity_test 0)

pico_add_extra_outputs(gpio_output_sanity_test)
