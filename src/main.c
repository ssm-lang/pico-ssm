#include <stdio.h>
#include <pico/stdlib.h>
#include <ssm-internal.h>

ssm_value_t v;

int main(void) {
  stdio_init_all();
  printf("Hello world!\n");
  return 0;
}
