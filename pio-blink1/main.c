/* Requires pico_stdlib in target_link_libraries  */
#include <pico/stdlib.h>
#include <stdio.h>

#include <ssm-internal.h>

int main(void) {
  set_sys_clock_pll(1536000000, 6, 2); // 128 MHz from the 12 MHz crystal

  stdio_init_all();

  ssm_platform_entry();

  printf("main() terminating\n");

  return 0;
}
