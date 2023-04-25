# RP2040 data sheet notes

## Peripherals

Each core, and read and write ports of the DMA controller, are masters on
the system's AHB-Lite (Advanced High-performance Bus) bus, which allow them to
interact with on-chip devices (including ROM, SRAM, PIO, etc). Lower bandwidth
devices like timers, UART, SPI, etc. are multiplexed over a single APB (Advanced
Peripheral Bus) bridge on the AHB-Lite bus. Each of the two cores and the DMA
controller's read and write ports can simultaneously interact with four
different devices on the AHB-Lite bus.

Here are the memory address space offsets for each family of devices:

- 0x00000000: ROM
- 0x10000000: XIP
  - 0x10000000: XIP access (cacheable, allocating)
    - cacheable means check cache for hit
    - allocating means update cache on miss
  - 0x11000000: XIP access cacheable, non-allocating
  - 0x12000000: XIP access non-cacheable, allocating
  - 0x13000000: XIP access non-cacheable, non-allocating
  - 0x15000000: use XIP cache as SRAM bank (only if XIP caching is disabled)
- 0x20000000: SRAM
  - address space is mirrored and striped in various ways to improve concurrency
- 0x40000000: APB peripherals
  - 0x40014000: IO_BANK0_BASE (GPIO user bank)
    - Each GPIO pin has a status and control register in this range
      - The status registers can be used to observe the state of each pin, but
        don't seem very useful
      - The control register is used to:
        - Configure the GPIO multiplexer, to configure which source output should come from
        - Configure the polarity of interrupt, input, and output
    - INTRn registers for "Raw interrupts"
      - I don't understand what this is for; is this to generate raw interrupts?
    - {PROC0,PROC1,DORMANT_WAKE}_{INTE,INTF,INTS} registers for enabling,
      forcing, and querying the status of each GPIO pin interrupt on core 0,
      core 1, and dormant wake (for timer hardware)
  - 0x40018000: IO_QSPI_BASE (GPIO QSPI bank)
  - 0x4001c000: PADS_BANK0_BASE
    - Used to configure electrical properties of each GPIO pad
  - 0x40020000: PADS_QSPI_BASE
  - 0x40024000: XOSC_BASE
  - 0x40030000: BUSCTRL_BASE
    - Used for configuring bus arbiter priority and reading performance counters
  - 0x40034000: UART0_BASE
  - 0x40038000: UART1_BASE
  - 0x40054000: TIMER_BASE
  - 0x4005c000: RTC_BASE
  - 0x40060000: ROSC_BASE
- 0x50000000: AHB-Lite peripherals
  - 0x50000000: DMA_BASE
  - 0x50100000: USBCTRL_BASE
  - 0x50200000: PIO0_BASE
    - Used to configure PIO0 device
  - 0x50300000: PIO1_BASE
  - 0x50400000: XIP_AUX_BASE
    - Used by DMA for streaming XIP data to memory without stalling
- 0xd0000000: IOPORT peripherals
  - 0xd0000000: SIO_BASE (single-cycle I/O)
    - Used for fast I/O local to/between the two cores, including:
        - CPUID, for figuring core identity
        - GPIO control, for reading and writing GPIO pins
          - A pin can only be written to ("driven") if its multiplexer is configured with SIO GPIO
          - A pin can be read at any time in a single cycle
        - Hardware spinlocks, to avoid expensive regular memory operations
        - Two unidirectional mailboxes (FIFOs), for sending data between cores
          - A processor can be configured to receive an interrupt each time data
            appears in its FIFO
        - Integer divider, for asynchronous (8-cycle) integer division
        - Interpolators, for programming and performing fixed arithmetic subroutines

## Interrupts

Each core has 32 interrupt inputs, but only the lower 26 IRQ signals are used;
26 to 31 are tied to zero (but can be forcibly triggered by writing to the NVIC
ISPR register).

I don't super understand this:

> Each NVIC has the same interrupts routed to it, with the exception of the GPIO
> interrupts: there is one GPIO interrupt per bank, per core. These are
> completely independent, so e.g. core 0 can be interrupted by GPIO 0 in bank 0,
> and core 1 by GPIO 1 in the same bank.

- 0: TIMER_IRQ_0
- 1: TIMER_IRQ_1
- 2: TIMER_IRQ_2
- 3: TIMER_IRQ_3
- 4: PWN_IRQ_WRAP
- 5: USBCTRL_IRQ
- 6: XIP_IRQ
- 7: PIO0_IRQ_0     (how is this configured?)
- 8: PIO0_IRQ_1     (how is this configured?)
- 9: PIO1_IRQ_0     (how is this configured?)
- 10: PIO1_IRQ_1    (how is this configured?)
- 11: DMA_IRQ_0     (how is this used?)
- 12: DMA_IRQ_1     (how is this used?)
- 13: IO_IRQ_BANK0  (aka GPIO)
- 14: IO_IRQ_QSPI   (aka GPIO)
- 15: SIO_IRQ_PROC0 (what is this for?)
- 16: SIO_IRQ_PROC1 (what is this for?)
- 17: CLOCKS_IRQ
- 18: SPI0_IRQ
- 19: SPI1_IRQ
- 20: UART0_IRQ
- 21: UART1_IRQ
- 22: ADC_IRQ_FIFO
- 23: I2C0_IRQ
- 24: I2C1_IRQ
- 25: RTC_IRQ
- 26-31: unused

## GPIO

There are 36 GPIO pins, divided into two banks: the user bank (pins 0--29), and
the QSPI bank (pins 30--35). Only the user bank's 30 pins should be used for
applications, since the QSPI bank pins are used for controlling the XIP flash
device.
