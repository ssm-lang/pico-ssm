#include "ssm-input.pio.h"
#include "ssm-output.pio.h"

#include <ssm-pio.h>

#include <hardware/clocks.h>
#include <hardware/pio.h>
#include <hardware/timer.h>

/*
// Use pio0 device by default
PIO pio = pio0;

// These dummy values will be populated by ssm_pio_init()
int sm_ctr = -1, sm_out = -1, sm_in = -1;

// TODO: adjust this value after configuring clocks
#define SYS_CLK_MHZ 125

static uint32_t ns_to_ctr(uint64_t ns) {
  uint64_t ticks = ns * SYS_CLK_MHZ / 1000, // i.e., sys_clk ticks
      ctr = ~(ticks / 8); // i.e., ssm_output_ctr counter increments
  return ctr;
}

static inline uint64_t ctr_to_ns(uint32_t ctr) {
  uint64_t ticks = ~ctr * 8,
           ns = ticks * 1000 / SYS_CLK_MHZ;

  // TODO: read from timer and combine with higher-order bits from that

  return ns;
}

int ssm_pio_init(uint32_t output_mask) {

  // TODO: clock configuration?

  // TODO: initialize ssm_input state machine + interrupt handlers

  sm_ctr = ssm_output_ctr_init(pio);
  sm_out = ssm_output_out_init(pio, output_mask);

  // Read lower 32 bits of system clock without
  uint32_t time_lo = timer_hw->timelr;
  uint32_t time_hi = timer_hw->timehr;
  uint64_t init_time_us = ((uint64_t)time_hi << 32u) | time_lo;

  // Initialize sm_ctr by pre-populating its TX FIFO, which it reads at start
  ssm_output_set_ctr(pio0, sm_ctr, ns_to_ctr(init_time_us * 1000));

  // TODO: add bitmask for input handler
  pio_set_sm_mask_enabled(pio, 1u << sm_ctr | 1u << sm_out, true);

  return 0;
}

void ssm_pio_schedule_output(uint64_t time_ns, uint32_t value_mask) {
  // TODO: check whether time_ns is already past, or too soon in the future.
  // In those cases, play it safe and don't bother scheduling rather than
  // potentially corrupting output (?)
  ssm_output_set_ctr(pio, sm_ctr, ns_to_ctr(time_ns));
  ssm_output_put(pio, sm_out, value_mask);
}
*/
