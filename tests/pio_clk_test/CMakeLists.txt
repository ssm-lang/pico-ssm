add_executable(pio_clk_test)

target_sources(pio_clk_test
    PRIVATE pio_clk_test.c
)

pico_generate_pio_header(pio_clk_test ${CMAKE_CURRENT_LIST_DIR}/pio-clk.pio)

target_link_libraries(pio_clk_test
    pico_stdlib
    hardware_pio
    hardware_clocks
)

pico_enable_stdio_usb(pio_clk_test 0)
pico_enable_stdio_uart(pio_clk_test 1)
pico_enable_stdio_semihosting(pio_clk_test 0)

pico_add_extra_outputs(pio_clk_test)
