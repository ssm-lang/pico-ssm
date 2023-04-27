add_executable(stdout_sanity_test)

target_sources(stdout_sanity_test
    PRIVATE stdout_sanity_test.c
)

target_link_libraries(stdout_sanity_test
    pico_runtime
    pico_time
    pico_stdio
    pico_stdlib
)

pico_enable_stdio_usb(stdout_sanity_test 0)
pico_enable_stdio_uart(stdout_sanity_test 1)
pico_enable_stdio_semihosting(stdout_sanity_test 0)

pico_add_extra_outputs(stdout_sanity_test)
