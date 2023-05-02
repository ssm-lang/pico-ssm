#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "ssm-output.pio.h"

#include <stdio.h>

#include <hardware/clocks.h>
#include <pico/stdlib.h>

uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)

#define PIN0 6
#define PIN1 7
#define PIN2 8

uint32_t make_mask(int p0, int p1, int p2) {
  return (!!p0) << PIN0 | (!!p1) << PIN1 | (!!p2) << PIN2;
}

uint32_t read_gpio(void) {
  return make_mask(1, 1, 1) & gpio_get_all();
}

void force_irq(PIO pio) {
  pio->irq_force = 1 << SSM_OUT_SET_IRQ;
}

int main(void) {
  stdio_init_all();

  sleep_ms(500);

  printf("\n\n=== ssm_output_out_test ===\n");

  clk_sys_hz = clock_get_hz(clk_sys);

  printf("System clock is running at %u Hz\n", clk_sys_hz);
  printf("At a period of %.3fns\n", (2. / clk_sys_hz) * 1000000000.);

  printf("Current GPIO situation: %08x\n", read_gpio());

  int sm = ssm_output_out_init(pio0, make_mask(1, 1, 1));
  if (sm < 0) {
    printf("Error! ssm_output_program_init() returned a neg: %d\n", sm);
    return 1;
  }

  pio_sm_set_enabled(pio0, sm, true);

  printf("Enabled state machine %d\n", sm);

  int x = 0;

  while (true) {
    int p0 = !!(x & 1), p1 = !!(x & 2), p2 = !!(x & 4);

    uint32_t val = make_mask(p0, p1, p2);

    printf("\n--------------\nStarting iteration %d...\n", x);
    printf("Current GPIO situation: %08x\n", read_gpio());

    printf("Sending value: %08x (%d, %d, %d)\n", val, p0, p1, p2);
    ssm_output_put(pio0, sm, val);

    printf("Current GPIO situation: %08x\n", read_gpio());

    if (x & 8) {
      printf("Sleeping for 500ms...\n");
      sleep_ms(500);
      printf("Woke up\n");
      printf("Current GPIO situation: %08x\n", read_gpio());
    }

    printf("Forcing IRQ...\n");
    force_irq(pio0);

    printf("Current GPIO situation: %08x\n", read_gpio());

    printf("Sleeping for 1000ms...\n");
    sleep_ms(1000);
    printf("Woke up\n");

    x++;
  }
  return 0;
}
