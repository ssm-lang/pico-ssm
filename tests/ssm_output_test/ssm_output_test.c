#include <stdio.h>

#include <hardware/clocks.h>
#include <pico/stdlib.h>

#include "pico/time.h"
#include "pico/types.h"
#include "ssm-output.pio.h"

#ifndef PICO_DEFAULT_LED_PIN
#error This program requires a board with a regular LED
#endif

uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)

#define ssm_pio_us_to_ticks(us) ((us) * (clk_sys_hz / 4 / 1000000u))
#define ssm_pio_ticks_to_ctr(ts) (-((ts)))

#define ssm_us(us) (ssm_pio_ticks_to_ctr(ssm_pio_us_to_ticks(us)))
#define ssm_ms(ms) ssm_us((ms) * 1000u)

int main(void) {
  stdio_init_all();

  sleep_ms(500);

  printf("\n\n=== Initialized ssm_output_test ===\n");

  clk_sys_hz = clock_get_hz(clk_sys);

  printf("System clock is running at %u Hz\n", clk_sys_hz);

  // Test with one pin first
  uint32_t pin_mask = 1u << PICO_DEFAULT_LED_PIN;

#if 0
  int sm = ssm_output_program_init(pio0, pin_mask);
  if (sm < 0) {
    printf("Error! ssm_output_program_init() returned a neg: %d\n", sm);
    return 1;
  }

  pio_sm_set_enabled(pio0, sm, true);

  uint32_t base = 0;
  const uint32_t epoch = 2000;

  absolute_time_t t, t_;
  uint32_t delta = 0;

  // Should loop every 4 seconds, blinking twice with a half-period of 400ms
  while (true) {

    t = get_absolute_time();

    printf("Epoch %d starting at %u microseconds... (delta of last iteration was %u us)\n", base / epoch, base, delta);

#define send(t, v)\
    pio_sm_put(pio0, sm, ssm_ms(t)); \
    pio_sm_put(pio0, sm, v); \

    // printf("    sending: 0x%08x @ %d (ticks = %u, ctr = %u)\n", v, t, ssm_pio_us_to_ticks((t) * 1000u), ssm_ms(t)); \

    send(base + 800u, 0);
    send(base + 1000u, 0xffffffff);
    send(base + 1200u, 0);
    send(base + 1600u, 0xffffffff);

    printf("Completed epoch...\n");

    base += epoch;

    t_ = get_absolute_time();
    delta = (uint32_t) absolute_time_diff_us(t, t_);

    sleep_us(epoch * 1000 - delta);
  }
#endif

  return 0;
}
