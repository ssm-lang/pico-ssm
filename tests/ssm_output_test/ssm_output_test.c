#include <stdio.h>

#include <hardware/clocks.h>
#include <pico/stdlib.h>

#include "ssm.pio.h"

#ifndef PICO_DEFAULT_LED_PIN
#error pio/hello_pio example requires a board with a regular LED
#endif

uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)

#define ssm_pio_us_to_ticks(us) ((us) * clk_sys_hz / 1000000)
#define ssm_pio_ticks_to_ctr(ts) (-(ts))

#define ssm_us(us) (ssm_pio_ticks_to_ctr(ssm_pio_us_to_ticks(us)))
#define ssm_ms(ms) ssm_us((ms) * 1000)

int main(void) {
  stdio_init_all();

  sleep_ms(500);

  printf("=== Initialized ssm_output_test ===\n");

  clk_sys_hz = clock_get_hz(clk_sys);

  printf("System clock is running at %u Hz\n", clk_sys_hz);

  // Test with one pin first
  uint32_t pin_mask = 1u << PICO_DEFAULT_LED_PIN;

  int sm = ssm_output_program_init(pio0, pin_mask);
  if (sm < 0) {
    printf("Error! ssm_output_program_init() returned a neg: %d\n", sm);
    return 1;
  }

  pio_sm_set_enabled(pio0, sm, true);

  uint32_t base = 0;
  const uint32_t epoch = 4000;

  // Should loop every 4 seconds, blinking twice with a half-period of 400ms
  while (true) {

    printf("Epoch starting at %u seconds...\n", base);

    pio_sm_put(pio0, sm, ssm_ms(base + 400));
    pio_sm_put(pio0, sm, pin_mask);
    pio_sm_put(pio0, sm, ssm_ms(base + 800));
    pio_sm_put(pio0, sm, 0);
    pio_sm_put(pio0, sm, ssm_ms(base + 1200));
    pio_sm_put(pio0, sm, pin_mask);
    pio_sm_put(pio0, sm, ssm_ms(base + 1600));
    pio_sm_put(pio0, sm, 0);

    printf("Completed epoch...\n");

    base += epoch;
    sleep_ms(epoch);
  }

  return 0;
}
