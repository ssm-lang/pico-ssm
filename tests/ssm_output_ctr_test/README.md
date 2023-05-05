# SSM Output Out Test

## Objectives

This test validates the out state machine from the `ssm-output.pio` program,
which is responsible for latching onto a CPU-submitted value until it receives
an IRQ, at which point it will write that value to the GPIO pins.

The mechanism by which the CPU submits a value is to first write the value to
a state machine's TX FIFO, and then writing the `pull block` into that state
machine's `SMn_INSTR` register, forcing it to reload the value from its TX FIFO.
The state machine is nominally blocked on `wait 1 irq IRQNUM`, so after
executing this CPU-issued instruction, it will resume waiting for an IRQ.
The purpose of this test is to establish that this mechanism works as
advertised.

We can force internal state machine interrupts to trigger by writing to the
`pio->irq_force` register; we use this mechanism to simulate an IRQ from another
state machine, for testing only.


## Conclusions

-   Writing to `SMn_INSTR` (using `pio_sm_exec()`) works as intended; the PIO
    program resumes blocking on `wait 1 irq IRQNUM` after being forced to
    execute `pull block` by the CPU.

-   The state machine successfully pulls the value from its TX FIFO.

-   Using `pio->irq_force` register works as advertised.
