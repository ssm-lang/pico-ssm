#ifndef SSM_RP2040_INTERNAL_H
#define SSM_RP2040_INTERNAL_H

#include <pico/sem.h>
#include <ssm-internal.h>

/*** Timers ***/

// Time types:
//
// ssm_time_t: 16 MHz, 64-bit
//
// microseconds (uint64_t or uint32_t): 1 MHz, 64-/32-bit
//
// pio_ctr_t: 16 MHz, 32-bit, bit-flipped
typedef uint32_t pio_ctr_t;

void ssm_rp2040_timer_init(void);

static inline ssm_time_t get_real_time(void) {
  uint32_t lo = timer_hw->timelr;
  uint32_t hi = timer_hw->timehr;
  return (((uint64_t)hi << 32) | lo) << 4;
}

static inline ssm_time_t time_to_us(uint64_t t) { return t >> 4; }
static inline uint64_t us_to_time(ssm_time_t t) { return t << 4; }

static inline pio_ctr_t time_to_ctr(ssm_time_t t) { return ~(pio_ctr_t)t; }
static inline ssm_time_t ctr_to_time(pio_ctr_t ctr) {
  ssm_time_t hi_mask = ~(((ssm_time_t)1 << 32) - 1);
  ssm_time_t rt = get_real_time();
  ssm_time_t hi = rt & hi_mask;
  uint32_t lo = ~ctr;
  if (lo & (1 << 31) && !(rt & (1 << 31)))
    hi -= (ssm_time_t)1 << 32;
  return hi | lo;
}

/*** Tick ***/
extern semaphore_t ssm_tick_sem;

/*** Alarm ***/

void ssm_rp2040_alarm_init(void);

#include <hardware/timer.h>
#define ALARM_NUM 0

/* Set the alarm time
 *
 * Note that this is only a 32-bit comparator so strange things
 * might happen with long delays
 */
static inline int set_alarm(ssm_time_t when) {
  // FIXME: check whether we've missed the alarm
  uint64_t when_us = time_to_us(when);
  timer_hw->alarm[ALARM_NUM] = (uint32_t)when_us;
  return 1;
}

static inline void unset_alarm(void) {
  hw_set_bits(&timer_hw->armed, 1 << ALARM_NUM);
}

/*** Memory allocation ***/

void ssm_rp2040_mem_init(void);

/*** I/O ***/

void ssm_rp2040_io_init(uint input_base, uint input_count, uint output_base,
                        uint output_count, ssm_value_t *input_out,
                        ssm_value_t *output_out);

// Poll for input, and schedule it if ready; returns non-zero if scheduled.
int ssm_rp2040_try_input(ssm_time_t next_time);

// Check output variable and forward any scheduled assignment to PIO.
void ssm_rp2040_forward_output(void);

#endif /* ifndef SSM_RP2040_INTERNAL_H */
