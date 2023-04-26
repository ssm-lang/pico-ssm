#include <hardware/gpio.h>
#include <pico/time.h>
#include <boards/pico.h>

// #define PIN PICO_DEFAULT_LED_PIN
#define PIN PICO_DEFAULT_UART_TX_PIN

int main(void) {

  gpio_init(PIN);
  gpio_set_dir(PIN, GPIO_OUT);

  for (;;) {
    for (int i = 0; i < 4; i++) {
      gpio_put(PIN, true);
      sleep_ms(200);
      gpio_put(PIN, false);
      sleep_ms(200);
    }
    for (int i = 0; i < 2; i++) {
      gpio_put(PIN, true);
      sleep_ms(400);
      gpio_put(PIN, false);
      sleep_ms(400);
    }
  }

  return 0;
}
