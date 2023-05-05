#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <pico/sem.h>
#include <pico/stdlib.h>
#include <ssm/pio/ssm-ctr.h>
#include <stdio.h>

#include "mypio.pio.h"
#include "ssm-output.pio.h"

// #define CLK_MHZ 128u
#define CLK_MHZ 125u
// #define CLK_MHZ 64u
// #define CLK_MHZ 48u
// #define CLK_MHZ 1u

#define DEATH_COUNT 8

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
  // Set sys_clk to run at 128MHz
  // set_sys_clock_pll(1536000, 6, 2);
  // Set sys_clk to run at 64MHz
  // set_sys_clock_pll(1536, 6, 4);

  // set_sys_clock_48mhz();
  //

  // This seems to work... (i.e., set the timer) but why doesn't
  // set_sys_clock_pll() work? And what is the hazard of doing this...?
  // Also this seems to hang the PIO device.
  // clock_configure(clk_sys,
  //                   CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
  //                   CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
  //                   128 * MHZ,
  //                   128 * MHZ);

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

  // Give PIO devices some time to wake up
  mysleep_until(time_us += 1000000);

  int death = -1;

  while (true) {
    // Wake up every 1ms
    uint32_t  wake_up_us = time_us + 5000u,
              wake_up_8us = wake_up_us / 8,
              wake_up_ctr = ~(wake_up_8us * CLK_MHZ),
              wake_up_ticks = wake_up_us * CLK_MHZ;
              // wake_up_ctr = ticks_to_ctr(wake_up_ticks);

    ssm_output_set_ctr(pio0, sm_ctr, wake_up_ctr);
    printf("\nIteration for time %u\n", time_us);
    printf("\tWake up (us): %u\n", wake_up_us);
    printf("\tWake up (ticks): %u\n", wake_up_ticks);
    printf("\tWake up (8us): %u\n", wake_up_8us);
    printf("\tWake up (ctr): %u\n", wake_up_ctr);

    uint8_t gpio_before = gpio_get(PIN);
    if (gpio_before)
      printf("\tCurrent GPIO (start of iteration): 1\n");
    else
      printf("\tCurrent GPIO (start of iteration): 0\n");

    mysleep_until(time_us += 500000);

    uint8_t gpio_mid = gpio_get(PIN);

    if (gpio_mid)
      printf("\tCurrent GPIO (start of iteration): 1\n");
    else
      printf("\tCurrent GPIO (start of iteration): 0\n");

    if (gpio_mid == gpio_before) {
      printf("!!!!!!! ERROR: gpio pin did not change after setting counter !!!!!!!\n");
      if (death < 0)
        death = DEATH_COUNT;
    }

    mysleep_until(time_us += 500000);

    if (gpio_mid != gpio_get(PIN)) {
      printf("!!!!!!! ERROR: gpio pin changed without setting counter !!!!!!!\n");
      if (death < 0)
        death = DEATH_COUNT;
    }

    if (death > 0)
      if (!--death)
        break;
  }

  printf("Game over\n");

  return 0;
}
