/* Requires pico_stdlib in target_link_libraries  */
#include <pico/stdlib.h>
#include <stdio.h>

#include <ssm-internal.h>

int main(void) {
  stdio_init_all();

  ssm_platform_entry();

  printf("main() terminating\n");

  return 0;
}
