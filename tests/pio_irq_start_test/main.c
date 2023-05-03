#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "mypio.pio.h"

#include <stdio.h>

#include <hardware/clocks.h>
#include <pico/stdlib.h>

uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)

#define PIN1 7
#define PIN2 8

uint32_t pin_mask = 1 << PIN1 | 1 << PIN2;

uint32_t read_gpio(void) {
  return pin_mask & gpio_get_all();
}

void force_irq(PIO pio) {
  pio->irq_force = (1 << CPU_TO_MYPIO_IRQ_1) | (1 << CPU_TO_MYPIO_IRQ_2);
}

int main(void) {
  stdio_init_all();

  sleep_ms(500);

  printf("\n\n=== pio_irq_start_test ===\n");

  clk_sys_hz = clock_get_hz(clk_sys);

  printf("System clock is running at %u Hz\n", clk_sys_hz);
  printf("At a period of %.3fns\n", (2. / clk_sys_hz) * 1000000000.);

  printf("Current GPIO situation: %08x\n", read_gpio());

  mypio_init(pio0, PIN1, PIN2);

  printf("Enabled state machines\n");

  printf("Current GPIO situation: %08x\n", read_gpio());

  force_irq(pio0);
  printf("Forced IRQ\n\n");

  return 0;
}
