#include <pico/stdio.h>
#include <pico/stdlib.h>

#include "ssm-rp2040-internal.h"

// TODO: hoist this to ctor, make sure it is called first
void ssm_rp2040_timer_init(void) {
  set_sys_clock_pll(1536000000, 6, 2); // 128 MHz from the 12 MHz crystal
  stdio_init_all(); // Reinitialize stdio, in case it was already initialized
}
