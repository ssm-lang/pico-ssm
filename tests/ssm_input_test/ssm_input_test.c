#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <pico/stdlib.h>
#include "ssm-input.pio.h"
#include "ssm-timer.pio.h"

#define PIN_IN 22
#define PIN_OUT PICO_DEFAULT_LED_PIN

uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)
uint32_t pin_mask = (1 << PIN_IN) | (1 << PIN_OUT);

uint32_t read_gpio(void) {
  return pin_mask & gpio_get_all();
}

int main(void) {

    uint32_t input_rx_fifo_out, timer_rx_fifo_out;
    int input_sm, timer_sm;

    stdio_init_all();

    printf("\n\n=== ssm_input_test ===\n");

    clk_sys_hz = clock_get_hz(clk_sys);

    printf("System clock is running at %u Hz\n", clk_sys_hz);
    printf("At a period of %.3fns\n", (2. / clk_sys_hz) * 1000000000.);

    printf("Current GPIO situation: %08x\n", read_gpio());

    gpio_set_pulls(PIN_IN, true, false); //Set to pull-up
    input_sm = ssm_input_program_start(pio0, PIN_IN, 32);
    timer_sm = ssm_pio_timer_program_start(pio0);

    pio_sm_set_enabled(pio0, timer_sm, true); //Enable timer

    printf("Enabled state machines\n");
    printf("Current GPIO situation: %08x\n", read_gpio());

    while (1) {
        if (pio_interrupt_get(pio0, SSM_INPUT_IRQ_NUM)) {
            pio_interrupt_clear(pio0, SSM_INPUT_IRQ_NUM);
            timer_rx_fifo_out = ssm_pio_timer_read(pio0, timer_sm);
            input_rx_fifo_out = pio_sm_get(pio0, input_sm);
            printf("New input %lu at time %lu\n", input_rx_fifo_out, timer_rx_fifo_out);
        }
    }

    return 0;
}
