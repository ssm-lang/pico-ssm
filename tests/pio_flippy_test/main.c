#include "mypio.pio.h"

#include <stdio.h>

#include <hardware/clocks.h>
#include <pico/stdlib.h>


uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)

int main(void) {
  stdio_init_all();

  sleep_ms(500);

  printf("\n\n=== pio_flippy_test ===\n");

  clk_sys_hz = clock_get_hz(clk_sys);

  printf("System clock is running at %u Hz\n", clk_sys_hz);
  printf("At a period of %.3fns\n", (2. / clk_sys_hz) * 1000000000.);

  int sm = mypio_program_init(pio0);
  if (sm < 0) {
    printf("Error! ssm_output_program_init() returned a neg: %d\n", sm);
    return 1;
  }

  pio_sm_set_enabled(pio0, sm, true);

  while(true) {
    printf("Just chillin\n");
    sleep_ms(1000);
  }

  return 0;
}