#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <hardware/pio_instructions.h>
#include <hardware/timer.h>
#include <pico/platform.h>
#include <pico/sem.h>
#include <pico/stdlib.h>
#include <stdio.h>

#include "mypio.pio.h"

#define CLK_MHZ 128u

uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)

#define PIO_PIN1 8
#define PIO_PIN2 9
#define DBG_PIN 10

int main(void) {
  set_sys_clock_pll(1536000000, 6, 2); // 128 MHz from the 12 MHz crystal
  stdio_init_all();

  printf("\n\n=== pio_latency_test ===\n");

  clk_sys_hz = clock_get_hz(clk_sys);
  int clk_peri_hz = clock_get_hz(clk_peri);

  printf("System clock is running at %u Hz\n", clk_sys_hz);
  printf("Peri clock is running at %u Hz\n", clk_peri_hz);

  printf("At a period of %.3fns\n", (2. / clk_sys_hz) * 1000000000.);

  int sm1 = mypio_init(pio0, PIO_PIN1);
  int sm2 = mypio_init(pio0, PIO_PIN2);
  uint32_t sm_mask = 1 << sm1 | 1 << sm2;
  __compiler_membar();
  printf("Force sm mask: %d\n", sm_mask);

  gpio_init(DBG_PIN);
  gpio_set_dir(DBG_PIN, GPIO_OUT);

  uint32_t time_us, init_ctr;

  while (1) {

    __compiler_membar();
    gpio_put(DBG_PIN, true);
    __compiler_membar();

    do {
      time_us = timer_hw->timerawl;

      init_ctr = ~(time_us * (CLK_MHZ / 8));

      pio_sm_put(pio0, sm1, init_ctr);

      pio_sm_put(pio0, sm2, init_ctr);

      pio_set_sm_mask_enabled(pio0, sm_mask, true);
    } while (0);
    __compiler_membar();
    gpio_put(DBG_PIN, false);
    __compiler_membar();

    busy_wait_at_least_cycles(100);
    busy_wait_at_least_cycles(16);

    pio_set_sm_mask_enabled(pio0, sm_mask, false);
    pio_sm_exec(pio0, sm1, pio_encode_set(pio_pins, 0));
    pio_restart_sm_mask(pio0, sm_mask);

    // Adjusted so that the loop is exactly 100 instructions
    busy_wait_at_least_cycles(16 + 9);
  }

  return 0;
}
