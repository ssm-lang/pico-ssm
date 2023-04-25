add_executable(mytest)

target_sources(mytest
    PRIVATE main.c
    PRIVATE pico-tick.c
    PRIVATE pico-throw.c
    PRIVATE blinky.c
)

target_link_libraries(mytest
    pico_stdlib
    hardware_pio
    hardware_flash
    ssm-runtime
    ssm-pio
)

pico_enable_stdio_usb(mytest 0)
pico_enable_stdio_uart(mytest 1)
pico_enable_stdio_semihosting(mytest 0)
pico_add_extra_outputs(mytest)
# pico_set_binary_type(mytest no_flash)
# pico_set_binary_type(mytest copy_to_ram)
