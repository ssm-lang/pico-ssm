#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <pico/sem.h>
#include <pico/stdlib.h>
#include <ssm/pio/ssm-ctr.h>
#include <stdio.h>

#include "mypio.pio.h"
#include "ssm-output.pio.h"

#define CLK_MHZ 125
// #define CLK_MHZ 1

uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)

#define PIN PICO_DEFAULT_LED_PIN
// #define PIN 10

void force_irq(PIO pio) { pio->irq_force = 1 << SSM_OUT_SET_IRQ; }

#define ALARM_IRQ TIMER_IRQ_0
#define ALARM_NUM 0

semaphore_t sem;

void timer_isr(void) {
  hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
  sem_release(&sem);
}

/* Set the alarm time
 *
 * Note that this is only a 32-bit comparator so strange things
 * might happen with long delays
 */
void set_alarm(uint32_t when) {
  timer_hw->alarm[ALARM_NUM] = when;
}

/*
 * Initialize our alarm
 */
void initialize_alarm(void) {
  hardware_alarm_claim(ALARM_NUM);
  hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
  irq_set_exclusive_handler(ALARM_IRQ, timer_isr);
  irq_set_enabled(ALARM_IRQ, true);
  set_alarm(~0);   // Far enough in the future to be irrelevant
}


void mysleep_until(uint32_t when_us) {
  // Assume that when_us is sufficiently far ahead in the future
  set_alarm(when_us);
  sem_acquire_blocking(&sem);
}

int main(void) {
  stdio_init_all();

  sleep_ms(500);

  printf("\n\n=== ssm_output_ctr_test ===\n");

  // Set sys_clk to run at 128MHz
  // set_sys_clock_pll(1536, 6, 2);

  clk_sys_hz = clock_get_hz(clk_sys);

  printf("System clock is running at %u Hz\n", clk_sys_hz);
  printf("At a period of %.3fns\n", (2. / clk_sys_hz) * 1000000000.);

  initialize_alarm();
  sem_init(&sem, 0, 1);

  int sm_ctr = ssm_output_ctr_init(pio0);
  int sm_dbg = mypio_init(pio0, PIN);

  printf("State machines: %d for ctr and %d for dbg\n", sm_ctr, sm_dbg);

  // Read lower 32 bits of system clock without
  uint32_t time_us = timer_hw->timerawl;

  // init_time is in microseconds, so we multiply by 128 to get the number of
  // sys_clk ticks. Then we use ticks_to_ctr to convert to PIO device counts,
  // i.e., divide by 8 and then bit-flip.
  ssm_output_set_ctr(pio0, sm_ctr, ticks_to_ctr(time_us * CLK_MHZ));

  printf("Initializing start time to %u (%u ticks)\n", time_us, ticks_to_ctr(time_us * CLK_MHZ));

  // Off to the races
  pio_set_sm_mask_enabled(pio0, (1 << sm_ctr) | (1 << sm_dbg), true);

  while (true) {
    // Wake up every 1ms
    ssm_output_set_ctr(pio0, sm_ctr, ticks_to_ctr((time_us + 500) * CLK_MHZ));
    printf("Iteration for time %d\n", time_us);

    mysleep_until(time_us += 1000000);
  }
  return 0;
}
