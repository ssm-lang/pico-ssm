# SSM PIO Timer Test

## Objectives

This test validates the `ssm-timer.pio` program, so that it can be used as
a reasonable source of truth when measuring the "local time" on the PIO
device.

To approximate a local timer on the CPU, we maintain a fake "counter" which is
just a `uint32_t` from which we decrement `125000000 / 8` every second. We
assume the drift due to printing can be ignored...

## Conclusions

-   PIO instruction delays seem to work as expected.

-   We can drain a state machine's RX FIFO from the CPU side by reading from it
    4 times (and throwing away the value).

-   The CPU and the PIO's counters do decrement at around the same rate.

-   As expected, there is quite a bit of drift between the CPU and PIO's counters.
    The drift grows very quickly if the CPU is stalled by printing text to UART
    (approx. 100K counts!). However, the drift seems to grow at a steady rate.

Questions:

-   It didn't seem like I needed to turn off optimizations to force the
    C program to read from the same register 4 times. Why? (Maybe this is
    something to do with the semantics of `static inline` functions?)
