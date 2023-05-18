#include <pico/sem.h>

#include <hardware/gpio.h> // FIXME: remove this!

#include "ssm-rp2040-internal.h"

// Hardware pin mappings.
// FIXME: get rid of these!

// #define LILYGO
// #define RP2040ZERO
#define PICO

#ifdef LILYGO
#define INPUT_PIN_BASE 6
#define INPUT_PIN_COUNT 2
#define OUTPUT_PIN_BASE 25
#define OUTPUT_PIN_COUNT 1
#endif

#ifdef RP2040ZERO
#define INPUT_PIN_BASE 4
#define INPUT_PIN_COUNT 2
#define OUTPUT_PIN_BASE 14
#define OUTPUT_PIN_COUNT 1
#endif

#ifdef PICO
#define INPUT_PIN_BASE 15
#define INPUT_PIN_COUNT 1
#define OUTPUT_PIN_BASE PICO_DEFAULT_LED_PIN
#define OUTPUT_PIN_COUNT 1
#endif

semaphore_t ssm_tick_sem;

int ssm_platform_entry(void) {
  ssm_rp2040_timer_init(); // NOTE: this should really be hoisted to ctors

  sem_init(&ssm_tick_sem, 0, 1);

  ssm_rp2040_mem_init();
  ssm_rp2040_alarm_init();

  // FIXME: remove this!
  ssm_value_t main_arg1, main_arg2;
  gpio_set_pulls(INPUT_PIN_BASE, true, false);
  ssm_rp2040_io_init(INPUT_PIN_BASE, INPUT_PIN_COUNT, OUTPUT_PIN_BASE, OUTPUT_PIN_COUNT, &main_arg1, &main_arg2);

  // Prepare the sslang program to start
  extern ssm_act_t *__enter_main(ssm_act_t *, ssm_priority_t, ssm_depth_t,
                                 ssm_value_t *, ssm_value_t *);
  // ssm_value_t main_arg1 = ssm_new_sv(ssm_marshal(0));
  // ssm_value_t main_arg2 = ssm_new_sv(ssm_marshal(0));
  ssm_value_t main_argv[2] = {main_arg1, main_arg2};

  ssm_activate(__enter_main(&ssm_top_parent, SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH,
                            main_argv, NULL));

  ssm_tick(); // gpio_pins configured here

  for (;;) {
    /* We must read real time before polling input to avoid this race condition:
     *
     *    next_time = next_event_time();
     *
     *    if (try_input()) {
     *      tick(); continue;
     *    }
     *
     *    // --- input appears here, after checking ---
     *
     *    real_time = get_real_time();
     *
     *    // Case: input_time < next_time < real_time
     *    // Correct action is to schedule input and then tick()
     *    // However, the following will tick() without scheduling input:
     *
     *    if (next_time < real_time) {
     *      tick(); continue;
     *    }
     *
     *    // etc.
     *
     * By reading real_time before polling input, we can guarantee that
     * real_time < input_time, making the above case impossible.
     */
    ssm_time_t next_time = ssm_next_event_time();
    ssm_time_t real_time = get_real_time();

    __compiler_memory_barrier(); // Ensure get_real_time() before polling input

    // printf("TICK: real %llu next %llu\n", real_time, next_time);

    if (ssm_rp2040_try_input(next_time) || next_time < real_time) {
      // Need to tick, either because of fresh input or because running behind
      ssm_tick();
      ssm_rp2040_forward_output();
    } else {
      // Need to sleep
      if (next_time == SSM_NEVER) {
        // No scheduled events; sleep forever
        sem_acquire_blocking(&ssm_tick_sem);
      } else {
        // Sleep until next_time
        if (set_alarm(next_time)) {
          sem_acquire_blocking(&ssm_tick_sem);
          unset_alarm();
          sem_reset(&ssm_tick_sem, 0);
        }
        // Did not set alarm, no need to block
      }
    }
  }

  return 0;
}

int main(void) {
  ssm_platform_entry();
  return 0;
}
