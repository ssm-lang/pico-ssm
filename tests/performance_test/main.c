#include <stdio.h>
#include "hardware/gpio.h"
#include <hardware/clocks.h>
#include "pico/stdlib.h"

#define INPUT_PIN 22
#define OUTPUT_PIN 0
#define SLEEP_TIME_US 100

void gpio_callback(uint gpio, uint32_t events) {
    gpio_put(OUTPUT_PIN, 1);
    busy_wait_us(SLEEP_TIME_US);
    gpio_put(OUTPUT_PIN, 0);
}

int main() {

    stdio_init_all();

    gpio_init(OUTPUT_PIN);
    gpio_set_dir(OUTPUT_PIN, GPIO_OUT);

    gpio_init(INPUT_PIN);
    gpio_set_dir(INPUT_PIN, GPIO_IN);
    gpio_pull_up(INPUT_PIN);
    gpio_set_input_enabled(INPUT_PIN, true);

    gpio_set_irq_enabled_with_callback(INPUT_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    for (;;)
        tight_loop_contents();

}