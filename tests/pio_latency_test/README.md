# PIO Latency test

## Objectives

This test measures the latency of the PIO counter initialization routine.

For that routine, we need to do 4 things:

1.  Read the lower 32 bits from the microsecond timer
2.  Convert from microseconds into counter values
3.  Put the initial counter into TX FIFO
4.  Start the PIO state machines

Step 3 is done twice, for two different state machines (input and output).

We would like to know how long it takes between the timer read and the PIO state
machine starting to execute, to see if there is skew we need to compensate for.

## Conclusions

The 4 steps described above (with two TX FIFO puts) take just under 200ns;
between the timer read and the PIO device starting to execute, just over 200ns
of time elapses.
