#include <stdio.h>
#include <pico/stdlib.h>
#include <ssm-platform.h>

int main(void) {
  stdio_init_all();

  printf("Hello world!\n");

  ssm_platform_entry();

  return 0;
}
