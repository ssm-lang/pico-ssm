set(test_name ssm_input_test)

add_executable(${test_name})

target_sources(${test_name}
    PRIVATE ssm_input_test.c
)

pico_generate_pio_header(${test_name} ${CMAKE_CURRENT_LIST_DIR}/ssm_input.pio)

target_link_libraries(${test_name}
    pico_stdlib
    hardware_pio
    hardware_clocks
    ssm-pio-header
)

pico_enable_stdio_usb(${test_name} 1)
pico_enable_stdio_uart(${test_name} 0)
pico_enable_stdio_semihosting(${test_name} 0)

pico_add_extra_outputs(${test_name})
