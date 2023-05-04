#include <stdio.h>
#include <pico/sem.h>
#include <pico/time.h>
#include <hardware/timer.h>
#include <hardware/irq.h>



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
 * Semaphore, etc.
 ******************************/

semaphore_t ssm_tick_sem;

static void timer_isr(void) {
  hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
  printf(".\n");
  sem_release(&ssm_tick_sem);
}

/* FIXME: The isr is running, but it's not doing the thing with the
   semapore properly */

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
ssm_time_t get_time(void) {
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
  hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
  irq_set_exclusive_handler(ALARM_IRQ, timer_isr);
  irq_set_enabled(ALARM_IRQ, true);
  timer_hw->alarm[ALARM_NUM] = (uint32_t) when;
}


int ssm_platform_entry(void) {

  printf("ssm_platform_entry()\n");

  hardware_alarm_claim(ALARM_NUM);
  //  hardware_alarm_set_callback(alarm_num, timer_isr);

  //  hardware_alarm_set_target(alarm_num,
  //			    delayed_by_us(get_absolute_time(),
  //					  (uint64_t) 100)); /* Nonsense time */

  
  ssm_mem_init(&alloc_page, &alloc_mem, &free_mem);


  /* Prepare the sslang program to start */
  
  extern ssm_act_t *__enter_main(ssm_act_t *, ssm_priority_t, ssm_depth_t,
                                 ssm_value_t *, ssm_value_t *);
  ssm_value_t nothing0 = ssm_new_sv(ssm_marshal(0));
  ssm_value_t nothing1 = ssm_new_sv(ssm_marshal(0));
  ssm_value_t main_argv[2] = {nothing0, nothing1};

  /* Start up main routine */
  ssm_activate(__enter_main(&ssm_top_parent, SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH,
                            main_argv, NULL));

  /* Run the program for time zero */
  ssm_tick();

  for (;;) {
    ssm_time_t next_time, wall_time;

    wall_time = get_time();
    next_time = ssm_next_event_time();

    printf("wall %llu next %llu\n", wall_time, next_time);

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
	printf("delaying to %llu\n", next_time);
	set_alarm(next_time);

	printf("wall %llu\n", get_time());
	
	// sem_acquire_blocking(&ssm_tick_sem);
	sem_acquire_block_until(&ssm_tick_sem, delayed_by_us(get_absolute_time(), 10000000));

	printf("unblocked at wall %llu\n", get_time());

	
        sem_reset(&ssm_tick_sem, 0);
      }
    }
  }
  
  return 0;
}
