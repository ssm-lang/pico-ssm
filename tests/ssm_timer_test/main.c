#include "ssm-timer.pio.h"

#include <stdio.h>

#include <hardware/clocks.h>
#include <pico/stdlib.h>


uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)

int main(void) {
  stdio_init_all();

  sleep_ms(500);

  printf("\n\n=== ssm_timer_test ===\n");

  clk_sys_hz = clock_get_hz(clk_sys);

  printf("System clock is running at %u Hz\n", clk_sys_hz);
  printf("At a period of %.3fns\n", (2. / clk_sys_hz) * 1000000000.);

  int sm = ssm_pio_timer_program_start(pio0);
  if (sm < 0) {
    printf("Error! ssm_output_program_init() returned a neg: %d\n", sm);
    return 1;
  }

  pio_sm_set_enabled(pio0, sm, true);

  uint32_t cpu_ctr = 0;
  uint32_t pio_last = 0;

  while(true) {
    sleep_ms(1000);

    cpu_ctr -= clk_sys_hz / 8;

    uint32_t pio_ctr = ssm_pio_timer_read(pio0, sm);

    printf("CPU time:  %u\n", cpu_ctr);
    printf("CPU diff:  %d\n", clk_sys_hz / 8);
    printf("PIO time:  %u\n", pio_ctr);
    printf("PIO diff:  %d\n", pio_last - pio_ctr);
    printf("CPU - PIO: %d\n\n", cpu_ctr - pio_ctr);

    pio_last = pio_ctr;
  }

  return 0;
}
