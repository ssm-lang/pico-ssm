# PIO debug test

## Objectives

This test aims to establish that we can use sideset pins to emit the program
counter throughout the execution of the PIO program, so that we can measure it
on a logic analyzer.

## Conclusions

-   Side set works, but is a little finnicky to configure.

    -   In the PIO assembly, use the `.side_set` directive to indicate how many
        pins should be used for side set. 

    -   Use `sm_config_sideset_pins()` to set the base pin.

    -   Use `sm_config_set_sideset()` to enable side set, indicating the number
        of pins used for side set (this should be exactly the same number
        specified in the `.side_set` directive). Note that if you don't do this,
        those side set bits will be interpreted as stall values, changing the
        timing of the program.

    -   Use `pio_gpio_init()` to configure each GPIO pin to be driven by the PIO
        device.

    -   Use `pio_sm_set_pindirs_with_mask()` to set the direction of each GPIO
        to output.

-   The PIO device nominally runs at 125MHz, which is way too fast for most
    logic analyzers (e.g., my Saleae Logic can only sample up to 24M/s). To use
    this technique to debug PIO programs, we may need to slow down the state
    machine using the clock divider, i.e., `sm_config_set_clkdiv_int_frac()`.

-   Side set only works on consecutive GPIO pins (like most other GPIO output
    functionality).
