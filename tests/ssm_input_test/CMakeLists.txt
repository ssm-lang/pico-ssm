add_executable(ssm_input_test)

target_sources(ssm_input_test
    PRIVATE ssm_input_test.c
)

target_link_libraries(ssm_input_test
    pico_stdlib
    hardware_pio
    ssm-pio-header
)

pico_enable_stdio_usb(ssm_input_test 0)
pico_enable_stdio_uart(ssm_input_test 1)
pico_enable_stdio_semihosting(ssm_input_test 0)

pico_add_extra_outputs(ssm_input_test)
