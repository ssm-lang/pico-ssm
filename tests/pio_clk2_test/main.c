#include <stdio.h>

#include <hardware/clocks.h>
#include <pico/stdlib.h>

#include "mypio.pio.h"

#ifndef PICO_DEFAULT_LED_PIN
#error This program requires a board with a regular LED
#endif

uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)

int main(void) {
  stdio_init_all();

  sleep_ms(500);

  printf("\n\n=== pio_clk2_test ===\n");

  clk_sys_hz = clock_get_hz(clk_sys);

  printf("System clock is running at %u Hz\n", clk_sys_hz);
  printf("Loading clock program, which will toggle pin %d\n", 15);
  printf("At a period of %.3fns\n", (2. / clk_sys_hz) * 1000000000.);

  mypio_program_init(pio0, 6, 8);

  while(true) {
    printf("Just chillin\n");
    sleep_ms(1000);
  }

  return 0;
}
