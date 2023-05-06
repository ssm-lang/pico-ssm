set(test_name pio_dma_test)

add_executable(${test_name})

target_sources(${test_name}
    PRIVATE main.c
)

target_link_libraries(${test_name}
    pico_stdlib
    hardware_pio
    hardware_dma
    hardware_pwm
    hardware_clocks
    ssm-pio-header
)

pico_enable_stdio_usb(${test_name} 1)
pico_enable_stdio_uart(${test_name} 0)
pico_enable_stdio_semihosting(${test_name} 0)

pico_add_extra_outputs(${test_name})