#include <stdio.h>

/* Requires pico_stdlib in target_link_libraries  */
#include <pico/stdlib.h>

#include <ssm-internal.h>

#define LED_PIN 14

int main(void) {
  int i;
  stdio_init_all();

  sleep_ms(2000); /* Time to connect the USB uart */

  printf("Serial blink 1 example\n");

   ssm_platform_entry();

  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  for (;;) {
    for (i = 0 ; i < 10 ; i++) {
      gpio_put(LED_PIN, 1);
      sleep_ms(100);
      gpio_put(LED_PIN, 0);
      sleep_ms(100);
    }
    printf("Serial Blink 1\n");
  }

  printf("main() terminating\n");

  return 0;
}
