#include <ssm-platform.h>
#include <stdio.h>

#include <hardware/pio.h>
#include <pico/stdlib.h>

#include "hello.pio.h"

void setup_pio(void) {
  // Copied from pico-examples
  PIO pio = pio0;
  uint offset = pio_add_program(pio, &hello_program);
  uint sm = pio_claim_unused_sm(pio, true);
  hello_program_init(pio, sm, offset, PICO_DEFAULT_LED_PIN);
  while (true) {
    pio_sm_put_blocking(pio, sm, 1);
    sleep_ms(500);
    pio_sm_put_blocking(pio, sm, 0);
    sleep_ms(500);
  }
}

int main(void) {
  stdio_init_all();

  printf("Hello world!\n");

  setup_pio();

  ssm_platform_entry();

  return 0;
}
