# PIO Latency test

## Objectives

This test measures the latency of the PIO counter initialization routine.

For that routine, we need to do 4 things:

1.  Read the lower 32 bits from the microsecond timer
2.  Convert from microseconds into counter values
3.  Put the initial counter into TX FIFO
4.  Start the PIO state machines

Step 3 might have to be done twice, for two different state machines (input and
output counter).

We would like to know this to see how much skew there is between the timer and
the PIO device.

## Conclusions

> **NOTE** for these tests, the system clock runs at 128MHz, i.e., a period of
> 15.625ns. The logic analyzer is configured to sample every 62.5ns (16MS/s).

### Putting to a single SM

| Step            | Cumulative SIO diff | Timer diff | Delta |
|-----------------|---------------------|------------|-------|
| Read timer      | 875ns (1.14MHz)     | 1us        | x     |
| Convert counter | 1688ns (592.59kHz)  | 1us        | 813ns |
| Put to TX FIFO  | 2437ns (410.26kHz)  | 2us        | 749ns |
| Enable PIO SMs  | 3312ns (301.89kHz)  | 2--3us     | 875ns |

PIO SM sets GPIO pin at the same time as END GPIO pin (probably too fast for my logic analyzer)

Note that latency goes up to 5313ns when I don't pre-compute the sm mask.

### Putting to both SMs

| Step            | Cumulative SIO diff | Timer diff | Delta |
|-----------------|---------------------|------------|-------|
| Read timer      | -                   | -          | x     |
| Convert counter | -                   | -          | -     |
| Put to TX FIFO  | -                   | -          | -     |
| Put to TX FIFO  | 3325ns (307.69kHz)  | 2--3us     | 888ns |
| Enable PIO SMs  | 4188ns (238.81kHz)  | 3--4us     | 863ns |

PIO SMs set GPIO pins around 375ns before END GPIO pin
