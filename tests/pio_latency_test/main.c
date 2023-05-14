#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <pico/sem.h>
#include <pico/stdlib.h>
#include <stdio.h>

#include "mypio.pio.h"

#define CLK_MHZ 128u

uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)

#define START_PIN 6
#define END_PIN 7
#define PIO_PIN1 8
#define PIO_PIN2 9

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
  printf("Force sm mask: %d\n", sm_mask);

  gpio_init(START_PIN);
  gpio_set_dir(START_PIN, GPIO_OUT);
  gpio_init(END_PIN);
  gpio_set_dir(END_PIN, GPIO_OUT);

  // Sleep for 1 second before doing everything

  uint32_t time_us, init_ctr;

  gpio_put(START_PIN, true);
  do {
    time_us = timer_hw->timerawl;

    init_ctr = ~(time_us * CLK_MHZ / 8);

    pio_sm_put(pio0, sm1, init_ctr);

    // pio_sm_put(pio0, sm2, init_ctr);

    pio_set_sm_mask_enabled(pio0, sm_mask, true);
  } while (0);
  gpio_put(END_PIN, true);

  // Read lower 32 bits of system clock again
  uint32_t time2_us = timer_hw->timerawl;
  uint32_t time3_us = timer_hw->timerawl;

  printf("\nCompleted initialization\n");
  printf("    First timer read (us):  %u\n", time_us);
  printf("    Counter:                %u\n", init_ctr);
  printf("    Second timer read (us): %u\n", time2_us);
  printf("    Third timer read (us):  %u\n", time3_us);
  printf("    second us - first us:   %u\n", time2_us - time_us);

  return 0;
}
