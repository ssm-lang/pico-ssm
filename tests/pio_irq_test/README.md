# PIO IRQ Test

## Objectives

This test checks whether the inter-state machine IRQ mechanism works. It is set
up like this:

```
CPU --- IRQ 5 --> SM1 --- IRQ 4 ---> SM2
```

The CPU sends IRQ 5 using the `irq_force` register; SM1 sends IRQ 4 using the
`irq` instruction.

Upon receiving the IRQ, the state machines blink some designated GPIO pins to
indicate that they received the IRQ.

We use a clock divider of 4096 or 8192 to ensure we can measure the GPIO pin
changes on a logic analyzer sampling at 16M/s.


## Conclusions

-   Inter-PIO IRQ seems to work, and they appear to take place within the span
    of one clock cycle. That is, IRQs are synchronized like this:

    ```
    SM1             | SM2
    --------------------------------
    nop             | wait 1 irq 5
    nop             | .
    irq nowait 5    | .
    nop             | nop
    nop             | nop
    ```

    Where the `nop`s represent other arbitrary (non-blocking) instructions.

-   There appears to be approx. 30us of latency between the execution of state
    machines (?!). If this is the case, that can greatly damage our timing
    precision. I need revisit the clk test case and run it on two simultaneous
    state machines to investigate what's going on.
