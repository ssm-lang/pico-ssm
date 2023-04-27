#include <boards/pico.h>
#include <hardware/gpio.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <stdio.h>

#define PIN PICO_DEFAULT_LED_PIN

int main(void) {
  stdio_init_all();

  // Use LED to indicate progress
  gpio_init(PIN);
  gpio_set_dir(PIN, GPIO_OUT);

  int i = 0;
  for (;;) {
    printf("Hello world %d\n", i++);

    sleep_ms(200);
    gpio_put(PIN, true);
    sleep_ms(200);
    gpio_put(PIN, false);
    sleep_ms(200);
    gpio_put(PIN, true);
    sleep_ms(200);
    gpio_put(PIN, false);
    sleep_ms(800);

    printf("Looping back now\n");
  }

  return 0;
}
