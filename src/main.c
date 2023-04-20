#include <ssm-platform.h>
#include <stdio.h>

#include <hardware/pio.h>
#include <pico/stdlib.h>

#include "hardware/sync.h"
#include "ssm.pio.h"
#include "pico/time.h"
#include "pico/types.h"

int alarm_num;
uint sm;

int i = 0;
static void alarm_cb(uint _alarm_num) {
  hardware_alarm_set_callback(alarm_num, alarm_cb);

  printf("Hi this is in the callback\n");

  // absolute_time_t t = get_absolute_time();
  // update_us_since_boot(&t, 500 * 1000);
  // hardware_alarm_set_target(alarm_num, t);
  hardware_alarm_set_target(alarm_num, delayed_by_us(get_absolute_time(), 500 * 1000));

  pio_sm_put_blocking(pio0, sm, i);
  i = !i;
}

void setup_pio(void) {
  // Copied from pico-examples
  // uint offset = pio_add_program(pio0, &hello_program);
  // sm = pio_claim_unused_sm(pio0, true);
  // hello_program_init(pio0, sm, offset, PICO_DEFAULT_LED_PIN);

  pio_sm_put_blocking(pio0, sm, 1);
  sleep_ms(200);
  pio_sm_put_blocking(pio0, sm, 0);
  sleep_ms(200);
  pio_sm_put_blocking(pio0, sm, 1);
  sleep_ms(200);
  pio_sm_put_blocking(pio0, sm, 0);
  sleep_ms(200);

  alarm_num = hardware_alarm_claim_unused(1);
  if (alarm_num < 0) {
      printf("Error: could not claim alarm!\n");
      exit(1);
  }
  hardware_alarm_set_callback(alarm_num, alarm_cb);

  // absolute_time_t t = get_absolute_time();
  // update_us_since_boot(&t, delayed_by_us(get_absolute_time(), 500 * 1000));
  hardware_alarm_set_target(alarm_num, delayed_by_us(get_absolute_time(), 500 * 1000));

  while (true) {
    __wfe();
  }
  /* while (true) {
    pio_sm_put_blocking(pio0, sm, 1);
    sleep_ms(500);
    pio_sm_put_blocking(pio0, sm, 0);
    sleep_ms(500);
  } */
}

int main(void) {
  stdio_init_all();

  printf("=====================\n");
  printf("Hello world!\n");
  printf("=====================\n");
  printf("\n");

  // setup_pio();

  ssm_platform_entry();

  return 0;
}
