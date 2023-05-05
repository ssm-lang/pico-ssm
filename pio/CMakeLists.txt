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
target_include_directories(ssm-pio-header INTERFACE include)

add_library(ssm-pio INTERFACE)
target_link_libraries(ssm-pio
    INTERFACE ssm-pio-header
)
