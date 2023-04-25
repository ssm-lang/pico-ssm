add_executable(ssm_output_test)

target_sources(ssm_output_test
    PRIVATE ssm_output_test.c
)

target_link_libraries(ssm_output_test
    pico_stdlib
    hardware_pio
    ssm-pio-header
)

pico_enable_stdio_usb(ssm_output_test 0)
pico_enable_stdio_uart(ssm_output_test 1)
pico_enable_stdio_semihosting(ssm_output_test 0)

pico_add_extra_outputs(ssm_output_test)
