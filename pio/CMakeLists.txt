# The assembled .pio program does not generate any compilation units, so we
# declare it an INTERFACE-only library. Any dependent that "link" against
# ssm-pio-header will be able to #include "ssm.pio.h".
#
# The ssm-pio-header library target primarily exists so that unit tests can
# include only ssm-pio and test each PIO program in isolation, without involving
# compilation units included in the ssm-pio library.
add_library(ssm-pio-header INTERFACE)
pico_generate_pio_header(ssm-pio-header ${CMAKE_CURRENT_LIST_DIR}/ssm-input.pio)
pico_generate_pio_header(ssm-pio-header ${CMAKE_CURRENT_LIST_DIR}/ssm-output.pio)
pico_generate_pio_header(ssm-pio-header ${CMAKE_CURRENT_LIST_DIR}/ssm-timer.pio)

add_library(ssm-pio)
target_sources(ssm-pio
    PRIVATE ssm-pio.c
)

target_include_directories(ssm-pio PUBLIC include)

target_link_libraries(ssm-pio
    pico_stdlib
    hardware_pio
    hardware_clocks
    ssm-pio-header
)
