#include <stdio.h>
#include <pico/sem.h>
#include <pico/time.h>
#include <hardware/timer.h>
#include <hardware/irq.h>

// Requires hardware_pio among the target_link_libraries

#include "hardware/pio.h"
#include "ssm-output.pio.h"

#include <ssm-internal.h>
#include <ssm-platform.h>



/******************************
 * Memory allocation
 ******************************/

#ifndef SSM_NUM_PAGES
#  define SSM_NUM_PAGES 32
#endif

static char mem[SSM_NUM_PAGES][SSM_MEM_PAGE_SIZE] = {0};
static size_t allocated_pages = 0;

static void *alloc_page(void) {
  if (allocated_pages >= SSM_NUM_PAGES)
    SSM_THROW(SSM_EXHAUSTED_MEMORY);
  return mem[allocated_pages++];
}

static void *alloc_mem(size_t size) { return malloc(size); }

static void free_mem(void *mem, size_t size) { free(mem); }



/******************************
 * Exception Handling
 ******************************/

void ssm_throw(ssm_error_t reason, const char *file, int line, const char *func) {
  printf("Threw error code %d at time: %016llx", reason, ssm_now());
  switch (reason) {
  case SSM_INTERNAL_ERROR:
    printf("Unforeseen internal error\n");
    break;
  case SSM_EXHAUSTED_ACT_QUEUE:
    printf("Tried to insert into full activation record queue\n");
    break;
  case SSM_EXHAUSTED_EVENT_QUEUE:
    printf("Tried to insert into full event queue");
    break;
  case SSM_EXHAUSTED_MEMORY:
    printf("Could not allocate more memory");
    break;
  case SSM_EXHAUSTED_PRIORITY:
    printf("Tried to exceed available recursion depth");
    break;
  case SSM_INVALID_TIME:
    printf(
        "Invalid time, e.g., scheduled delayed assignment at an earlier time");
    break;
  default:
    printf("Unknown/platform-specific error");
    break;
  }
  exit(reason);
}

#define ALARM_IRQ TIMER_IRQ_0
#define ALARM_NUM 0

/******************************
 * Semaphore and timer interrupt
 ******************************/

semaphore_t ssm_tick_sem;

static void timer_isr(void) {
  // Compulsory: clear the interrupt
  hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
  sem_release(&ssm_tick_sem); // Wake up the tick loop
}

/******************************
 * Low-level 64-bit timer access
 *
 * See the rp2040 datasheet, page 557
 *****************************/

/* Read the 64-bit time value
 *
 * Potentially unsafe if the other core is simultaneously trying
 * to read the timer at the same time
 */
ssm_time_t get_real_time(void) {
  uint32_t lo = timer_hw->timelr;
  uint32_t hi = timer_hw->timehr;
  return ( (ssm_time_t) hi << 32u ) | lo;
}

/* Set the alarm time
 *
 * Note that this is only a 32-bit comparator so strange things
 * might happen with long delays
 */
void set_alarm(ssm_time_t when)
{
  timer_hw->alarm[ALARM_NUM] = (uint32_t) when;
}

/*
 * Initialize our alarm
 */
void initialize_alarm()
{
  hardware_alarm_claim(ALARM_NUM);
  
  hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
  irq_set_exclusive_handler(ALARM_IRQ, timer_isr);
  irq_set_enabled(ALARM_IRQ, true);
  set_alarm((ssm_time_t) ~0);   // Far enough in the future to be irrelevant
}

#define LED_PIN 14

int ssm_platform_entry(void) {

  // Initialize the semaphore
  sem_init(&ssm_tick_sem, 0, 1);

  // Configure our alarm
  initialize_alarm();

  int sm;
  if ((sm = ssm_output_out_init(pio0, 1 << LED_PIN)) < 0) {
    printf("Error! ssm_output_program_init() returned %d\n", sm);
    return 1;
  }
  
  pio_sm_set_enabled(pio0, sm, true);  
  
  ssm_mem_init(&alloc_page, &alloc_mem, &free_mem);

  // Prepare the sslang program to start
  extern ssm_act_t *__enter_main(ssm_act_t *, ssm_priority_t, ssm_depth_t,
                                 ssm_value_t *, ssm_value_t *);
  ssm_value_t nothing0 = ssm_new_sv(ssm_marshal(0));
  ssm_value_t gpio_output = ssm_new_sv(ssm_marshal(0));
  ssm_value_t main_argv[2] = {nothing0, gpio_output};
  
  ssm_activate(__enter_main(&ssm_top_parent, SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH,
                            main_argv, NULL));

  // Run the program for time zero
  ssm_tick();

  // Run the program or sleep if we're too early
  for (;;) {
    ssm_time_t real_time = get_real_time();
    ssm_time_t next_time = ssm_next_event_time();

    printf("real %llu next %llu\n", real_time, next_time);

    __compiler_memory_barrier();

    // Write to the GPIO if we're past the deadline for updating it
    if (real_time >= ssm_to_sv(gpio_output)->later_time) {
      ssm_output_put(pio0, sm,
		     ssm_unmarshal(ssm_to_sv(gpio_output)->later_value));
      pio0->irq_force = 1 << SSM_OUT_SET_IRQ; // force the IRQ
    }

    if (next_time <= real_time)
      ssm_tick(); // We're behind: catch up to reality
    else if (next_time == SSM_NEVER)
      sem_acquire_blocking(&ssm_tick_sem); // Sleep until an external event
    else {
      set_alarm(next_time);                // Set the alarm to wake us up
      sem_acquire_blocking(&ssm_tick_sem); // Sleep until the alarm or earlier
      sem_reset(&ssm_tick_sem, 0);
    }    
  }
  
  return 0;
}
