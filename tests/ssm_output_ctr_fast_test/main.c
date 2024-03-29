#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <pico/sem.h>
#include <pico/stdlib.h>
#include <stdio.h>

#include "mypio.pio.h"
#include "ssm-output.pio.h"

#define CLK_MHZ 128u

uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)

#define PIN 6

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
  set_sys_clock_pll(1536000000, 6, 2); // 128 MHz from the 12 MHz crystal

  stdio_init_all();

  sleep_ms(500);

  printf("\n\n=== ssm_output_ctr_test ===\n");

  clk_sys_hz = clock_get_hz(clk_sys);
  int clk_peri_hz = clock_get_hz(clk_peri);

  printf("System clock is running at %u Hz\n", clk_sys_hz);
  printf("Peri clock is running at %u Hz\n", clk_peri_hz);

  printf("At a period of %.3fns\n", (2. / clk_sys_hz) * 1000000000.);

  initialize_alarm();
  sem_init(&sem, 0, 1);

  int sm_ctr = ssm_output_ctr_init(pio0);
  int sm_dbg = mypio_init(pio0, PIN);
  int sm_mask = 1 << sm_ctr | 1 << sm_dbg;

  printf("State machines: %d for ctr and %d for dbg (mask = %d)\n", sm_ctr, sm_dbg, sm_mask);

  // Read lower 32 bits of system clock without worrying about higher bits.
  uint32_t time_us = timer_hw->timerawl;

  // init_time is in microseconds, so we multiply by CLK_MHZ to get the number
  // of sys_clk ticks. Then we divide by 8 and bit-flip to convert to counter
  // system that the ssm_output_ctr program uses.
  uint32_t init_ctr = ~(time_us << 4);

  ssm_output_set_ctr(pio0, sm_ctr, init_ctr);

  // Off to the races
  pio_set_sm_mask_enabled(pio0, sm_mask, true);

  printf("Initializing start time to %u (%u ctr)\n", time_us, init_ctr);

  // Give PIO devices some time to wake up
  mysleep_until(time_us += 1000000);

  // Wake up every 1ms
  while (true) {
    uint64_t  wake_up_us = time_us + 50u,
              wake_up_ticks = wake_up_us * CLK_MHZ,
              wake_up_ctr = ~(wake_up_ticks / 8);

    ssm_output_set_ctr(pio0, sm_ctr, (uint32_t) wake_up_ctr);

    mysleep_until(time_us += 200);
  }

  printf("Game over\n");

  return 0;
}
