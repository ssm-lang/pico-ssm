#include <stdio.h>
#include <stdlib.h>
#include "hardware/gpio.h"
#include <hardware/clocks.h>
#include <pico/stdlib.h>

#define INPUT_PIN 22
#define OUTPUT_PIN 0
#define SLEEP_TIME_US 100

int main() {

    uint32_t start_time, delta_time, total_time, button_presses;

    stdio_init_all();

    gpio_init(OUTPUT_PIN);
    gpio_set_dir(OUTPUT_PIN, GPIO_OUT);

    gpio_init (INPUT_PIN);
    gpio_set_dir(INPUT_PIN, GPIO_IN);
    gpio_pull_up(INPUT_PIN);
    gpio_set_input_enabled(INPUT_PIN, true);

    total_time = 0;
    button_presses = 0;

    for (;;) {

        //Spin and wait for the input to be enabled
        while (gpio_get(INPUT_PIN))
            ;

        // Send a 100us pulse
        start_time = time_us_32();
        gpio_put(OUTPUT_PIN, 1);
        sleep_us(SLEEP_TIME_US);
        gpio_put(OUTPUT_PIN, 0);
        delta_time = time_us_32() - start_time;
        button_presses++;
        total_time += delta_time;

        //Print the results
        printf("Pulse len: %lu Avg. pulse len: %lu\n", delta_time, total_time / button_presses);

        //Wait for the button to reset
        while (!gpio_get(INPUT_PIN))
            ;

    }

}