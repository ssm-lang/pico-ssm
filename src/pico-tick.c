#include "hardware/sync.h"
#include "hardware/timer.h"
#include "pico/platform.h"
#include <ssm-internal.h>
#include <ssm-platform.h>
#include <pico/sem.h>
#include <stdio.h>

#ifndef SSM_NUM_PAGES
#define SSM_NUM_PAGES 32
#endif

semaphore_t ssm_tick_sem;

static void wake_up(uint _alarm_num) {
  sem_release(&ssm_tick_sem);
}

static void setup_entry_point(void) {
  // NOTE: this is kind of a hack. Ideally the user program is able to specify
  // what it wants the entry point to be. But for now, we assume that there's
  // some entry point named 'main' whose activation record is set up using
  // '__enter_main'.
  extern ssm_act_t *__enter_main(ssm_act_t *, ssm_priority_t, ssm_depth_t,
                                 ssm_value_t *, ssm_value_t *);
  ssm_value_t nothing0 = ssm_new_sv(ssm_marshal(0));
  ssm_value_t nothing1 = ssm_new_sv(ssm_marshal(0));
  ssm_value_t main_argv[2] = {nothing0, nothing1};

  /* Start up main routine */
  ssm_activate(__enter_main(&ssm_top_parent, SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH,
                            main_argv, NULL));
}

static char mem[SSM_NUM_PAGES][SSM_MEM_PAGE_SIZE] = {0};
static size_t allocated_pages = 0;

static void *alloc_page(void) {
  if (allocated_pages >= SSM_NUM_PAGES)
    SSM_THROW(SSM_EXHAUSTED_MEMORY);
  return mem[allocated_pages++];
}

static void *alloc_mem(size_t size) { return malloc(size); }

static void free_mem(void *mem, size_t size) { free(mem); }

int ssm_platform_entry(void) {

  // Claim an alarm
  int alarm_num = hardware_alarm_claim_unused(1);
  if (alarm_num < 0) {
      printf("Error: could not claim alarm!\n");
      return -1;
  }

  ssm_mem_init(&alloc_page, &alloc_mem, &free_mem);

  sem_init(&ssm_tick_sem, 0, 1);

  setup_entry_point();

  // NOTE: we are assuming no input events at time 0.
  ssm_tick();

  for (;;) {
    ssm_time_t next_time, wall_time;

    wall_time = time_us_64() * 1000;
    next_time = ssm_next_event_time();

    __compiler_memory_barrier();

    if (next_time <= wall_time) {
      ssm_tick();
    } else {
      if (next_time == SSM_NEVER) {
        // Nothing to do; sleep indefinitely

        // Consider terminating "properly" at this point if there are no more
        // active processes, i.e., the program has terminated. This code is not
        // well-tested, so for now, when the program terminates, we send the
        // pico into endless slumber.
        //
        // if (!ssm_active())
        //   break;

        sem_acquire_blocking(&ssm_tick_sem);
      } else {

        // TODO: what order to call these in??
        hardware_alarm_set_callback(alarm_num, wake_up);
        if (hardware_alarm_set_target(alarm_num, from_us_since_boot(next_time / 1000))) {
          // Target missed
        }

        sem_acquire_blocking(&ssm_tick_sem);

        sem_reset(&ssm_tick_sem, 1);
      }
    }
  }

  return 0;
}
