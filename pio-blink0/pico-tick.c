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


/******************************
 * PIO Output Management
 ******************************/

#define OUTPUT_PIO pio0

int pio_output_sm_ctr; // State machine for counter/alarm
int pio_output_sm_out; // State machine for output

static inline void ssm_output_init(uint32_t pin_mask) {
  pio_output_sm_ctr = ssm_output_ctr_init(OUTPUT_PIO);
  pio_output_sm_out = ssm_output_out_init(OUTPUT_PIO, pin_mask);

  pio_set_sm_mask_enabled(pio0,
			  (1 << pio_output_sm_ctr) | (1 << pio_output_sm_out),
			  true);

  // Initialize the timer count based on the current hardware counter value
  uint32_t lo = timer_hw->timelr;
  ssm_output_set_ctr(pio0, pio_output_sm_ctr, ~( lo << 4));
  
  // printf("Initialized counter to %d\n", lo);
}

static inline void ssm_pio_schedule_output(uint64_t time_ns,
					   uint32_t value_mask) {
  ssm_output_set_ctr(OUTPUT_PIO, pio_output_sm_ctr, (uint32_t) time_ns);
  ssm_output_put(OUTPUT_PIO, pio_output_sm_out, value_mask);
}

#define LED_PIN 14

int ssm_platform_entry(void) {

  // Set up the PIO output system
  ssm_output_init(1 << LED_PIN);  

  // Initialize the semaphore
  sem_init(&ssm_tick_sem, 0, 1);

  // Configure our alarm
  initialize_alarm();


  
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

    // printf("real %llu next %llu\n", real_time, next_time);

    __compiler_memory_barrier();

    // Update the scheduled GPIO
    uint64_t gpio_later = ssm_to_sv(gpio_output)->later_time;
    if (gpio_later != SSM_NEVER) {
      /* printf("scheduled %d at %llu\n",
	     ssm_unmarshal(ssm_to_sv(gpio_output)->later_value),
	     gpio_later); */

      gpio_later = gpio_later << 4;
      ssm_pio_schedule_output(~gpio_later,
			      ssm_unmarshal(ssm_to_sv(gpio_output)->later_value));
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

/*

125 MHz main clock?

8 cycles per PIO clock

1 us timer resolution =


Reported by hello_48MHz.c,

pll_sys  = 125000kHz  = 125 MHz
pll_usb  = 48000kHz
rosc     = 6068kHz
clk_sys  = 125000kHz
clk_peri = 125000kHz
clk_usb  = 48000kHz
clk_adc  = 48000kHz
clk_rtc  = 47kHz

clk_ref  = 12001kHz   = 12 MHz



The PIO runs at the system clock rate 


Using hello_128MHz.c,

Hello, world!
pll_sys  = 125000kHz
pll_usb  = 48000kHz
rosc     = 6068kHz
clk_sys  = 125000kHz
clk_peri = 125001kHz
clk_usb  = 48000kHz
clk_adc  = 48000kHz
clk_rtc  = 47kHz
clk_ref  = 12001kHz

pll_sys  = 128000kHz
pll_usb  = 48000kHz
rosc     = 6068kHz
clk_sys  = 128000kHz
clk_peri = 48000kHz
clk_usb  = 48000kHz
clk_adc  = 48000kHz
clk_rtc  = 47kHz
clk_ref  = 12001kHz

Hello, 128MHz

 */
