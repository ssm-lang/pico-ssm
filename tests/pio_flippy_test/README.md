# PIO Flippy Test

## Objectives

This test validates that wraparound works as expected. In particular, it toggles
between two states each time a 32-bit counter reaches zero, but continues
decrementing the counter. If the counter wrapped around to 2^32-1, as we expect
it to, then we should see it change state every 2^32 decrements.

The state is made visible by side setting the LED pin.

## Conclusions

The light changes state approximately every 68 seconds, which is what we expect,
i.e., `2^32 * (2 / 125000000)`.
