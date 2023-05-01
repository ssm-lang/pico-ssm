# PIO clock test

## Objectives

This test measures the clock rate of the PIO device.

## Conclusions

-   The clock rate is about 125MHz, i.e., 1 instruction every 8ns. Using an
    oscilloscope, we find that the period of the output square wave is
    approximately 16ns, i.e., 2 * (1 / 125MHz).

-   The GPIO pins seem to take a little while to reach voltage; if I slow the
    clock rate, the measured voltage increases. Even at clkdiv = 8, the output
    wave form does not really resemble a square wave.

| clkdiv | period | amplitude |
|--------|--------|-----------|
| 1      | 16ns   | 750mV     |
| 2      | 32ns   | 2V        |
| 3      | 48ns   | 3.3V      |
| 4      | 64ns   | 3.8V      |
| 5      | 80ns   | 3.8V      |
| 6      | 96ns   | 3.8V      |
| 7      | 112ns  | 3.8V      |
| 8      | 128ns  | 3.8V      |
