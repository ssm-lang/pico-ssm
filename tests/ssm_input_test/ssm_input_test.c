#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "ssm_input.pio.h"
#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <pico/stdlib.h>

#define PIN_IN 22
#define PIN_OUT PICO_DEFAULT_LED_PIN

uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)
uint32_t pin_mask = (1 << PIN_IN) | (1 << PIN_OUT);

uint32_t read_gpio(void) {
  return pin_mask & gpio_get_all();
}


int main(void) {

    uint32_t rx_fifo_out;

    stdio_init_all();

    sleep_ms(10);

    printf("\n\n=== ssm_input_test ===\n");

    clk_sys_hz = clock_get_hz(clk_sys);

    printf("System clock is running at %u Hz\n", clk_sys_hz);
    printf("At a period of %.3fns\n", (2. / clk_sys_hz) * 1000000000.);

    printf("Current GPIO situation: %08x\n", read_gpio());

    gpio_set_pulls(PIN_IN, true, false);
    ssm_input_init(pio0, PIN_IN, PIN_OUT);

    printf("Enabled state machines\n");
    printf("Current GPIO situation: %08x\n", read_gpio());

    while (1) {
        rx_fifo_out = pio_sm_get_blocking(pio0, 0);
        printf("Input switched to %u at time %llu\n", rx_fifo_out, time_us_64());
    }

    return 0;
}
