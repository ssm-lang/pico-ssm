#include <stdio.h>
#include <stdlib.h>
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

#define US_TO_PIO(t) (~((t) << 4))

static inline void ssm_output_init(uint32_t pin_mask) {
  pio_output_sm_ctr = ssm_output_ctr_init(OUTPUT_PIO);
  pio_output_sm_out = ssm_output_out_init(OUTPUT_PIO, pin_mask);

  // Initialize the timer count based on the current hardware counter value
  
  // FIXME: We'd like this to be even more precise since we don't know
  // the exact latency between reading the hardware counter
  // and starting the state machines
  
  uint32_t lo = timer_hw->timelr;
  ssm_output_set_ctr(pio0, pio_output_sm_ctr, US_TO_PIO(lo));

  pio_set_sm_mask_enabled(pio0,
			  (1 << pio_output_sm_ctr) | (1 << pio_output_sm_out),
			  true);
}

static inline void ssm_pio_schedule_output(uint32_t pio_time,
					   uint32_t value_mask) {
  ssm_output_put(OUTPUT_PIO, pio_output_sm_out, value_mask);
  ssm_output_set_ctr(OUTPUT_PIO, pio_output_sm_ctr, pio_time);
}

static inline void ssm_pio_output_now(uint32_t value) {
  ssm_output_put(OUTPUT_PIO, pio_output_sm_out, value);
  OUTPUT_PIO->irq_force = 1 << SSM_OUT_SET_IRQ;
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

    if (next_time <= real_time) {
      ssm_tick(); // We're behind: catch up to reality

      // Update the scheduled GPIO
      uint64_t gpio_later = ssm_to_sv(gpio_output)->later_time;
      if (gpio_later != SSM_NEVER) {
	
	uint32_t toschedule = (uint32_t) gpio_later;
	ssm_pio_schedule_output(US_TO_PIO(toschedule),
				ssm_unmarshal(ssm_to_sv(gpio_output)->later_value));
	// printf("PIO  %u next %u\n", timer_hw->timelr, toschedule);
      }

    } else if (next_time == SSM_NEVER)
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


Measured times at which the PIO counters were started

hardware_clock_skew: 202694
hardware_clock_skew: 205574
hardware_clock_skew: 205470
hardware_clock_skew: 205417
hardware_clock_skew: 205592

Fairly consistent, but certainly not to the us.

Certainly necessary to try to synchronize clocks

Time it takes to read the internal clock and set the PIO clock

hardware_clock_skew: 202693
hardware_clock_skew2: 202695

hardware_clock_skew: 206090
hardware_clock_skew2: 206091

hardware_clock_skew: 205392
hardware_clock_skew2: 205393

hardware_clock_skew: 205633
hardware_clock_skew2: 205635

Looks like 1-2 us, which is about what I expected


main a gpio =
  let timer = new ()
  after 2000000, timer <- ()
  loop
    after     100, gpio <- 16384 - deref gpio // will work down to 100
    after  250000, timer <- ()  // Fails at 200000 and below
    wait timer

gives

real 33000004 next 33000000
PIO to 33000100 at 33000176
real 33000312 next 33000100
real 33000496 next 33250000
real 33250004 next 33250000
PIO to 33250100 at 33250152
real 33250315 next 33250100
real 33250493 next 33500000
real 33500004 next 33500000
PIO to 33500100 at 33500181
real 33500316 next 33500100
real 33500487 next 33750000
real 33750004 next 33750000
PIO to 33750100 at 33750151
real 33750319 next 33750100
real 33750493 next 34000000
real 34000004 next 34000000

Which seems like it shouldn't work since the PIO scheduled time
is after the master time.


When I set the gpio to immediately 0 before scheduling the update,

  loop
    after    7500, gpio <- 16384 - deref gpio
    after  250000, timer <- ()  // Fails at 200000 and below
    wait timer

I see a positive pulse of 39.88 ms

  loop
    after   10000, gpio <- 16384 - deref gpio
    after  250000, timer <- ()  // Fails at 200000 and below
    wait timer

gives a positive pulse of 37.38 ms

  loop
    after   20000, gpio <- 16384 - deref gpio
    after  250000, timer <- ()  // Fails at 200000 and below
    wait timer

gives a positive pulse of 24.65 ms +- 0.3 ms






Turn on LED when scheduled; turn off at scheduled time

  loop
    after   30000, gpio <- 0 // 16384 - deref gpio
    after  250000, timer <- ()  // Fails at 200000 and below
    wait timer


off for 17.6 ms; on for 232 ms; period = 250 ms (consistent with timer)


  loop
    after   70000, gpio <- 0 // 16384 - deref gpio
    after  250000, timer <- ()  // Fails at 200000 and below
    wait timer

on all the time


  loop
    after    1000, gpio <- 0 // 16384 - deref gpio
    after  250000, timer <- ()  // Fails at 200000 and below
    wait timer

off for 38 ms; on for 203 ms; 250 ms period

real 10000004 next 10000000
PIO  10000156 next 10001000
real 10000323 next 10001000
real 10001004 next 10001000
real 10001195 next 10250000
real 10250004 next 10250000
PIO  10250150 next 10251000

Adding printing changed the off width to 43 ms, which appears to be stable

stable -> suggests clock speeds are synchronized (no drift), but skewed


It should only be on for something like 1ms instead of 43 ms, so something's very off

On is when the the scheduled time is entered

E.g,

PIO  10000156
next 10001000

should only be 1 ms or so

but it's more like 208 ms

Problem was that John was doing a nonblocking read from the 
input FIFO when starting up the ssm_output_ctr SM but I was writing it after
it had started.


Switching that makes

  loop
    after    1000, gpio <- 0 // 16384 - deref gpio
    after  250000, timer <- ()  // Fails at 200000 and below
    wait timer

Now pulses high for 934-937 us

LESSON LEARNED: really need to get these clocks carefully synchronized

Reports things like this:

PIO  13250183 next 13251000

which corresponds to 817 us: pretty close


I removed the print statements and the pulse width high is now
996 us


Problem was that I wasn't actually synchronizing the two clock times;
they were about 200 ms off (i.e., the time it took to start the PIO
after reset).

Removing all the printing stuff, etc. make it work as fast as 20 kHz


 */
