#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include <hardware/clocks.h>
#include <pico/stdlib.h>
#include "ssm-input.pio.h"


//Max size for input_buffer_bits is 14

#define INPUT_BUFFER_BITS 6
#define INPUT_BUFFER_SIZE (1 << INPUT_BUFFER_BITS)
#define PIN_IN 22
#define PIN_OUT PICO_DEFAULT_LED_PIN

uint32_t input_buffer[INPUT_BUFFER_SIZE] __attribute__((aligned(INPUT_BUFFER_SIZE * sizeof(uint32_t))));
uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)

int pio_dma_init(PIO pio, uint sm, uint32_t *input_buffer) {

    int dma_channel = dma_claim_unused_channel(true);
    
    dma_channel_config c = dma_channel_get_default_config(dma_channel);
    //Read a word at a time
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    //Reads are always from the FIFO, don't increment
    channel_config_set_read_increment(&c, false);
    //Writes should increment
    channel_config_set_write_increment(&c, true);
    //Get transfers from the RX FIFO
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));
    //Make writes go in a ring
    channel_config_set_ring(&c, true, INPUT_BUFFER_BITS * sizeof(uint32_t));

    //Start reading from the state machine to capture_buffer immediately
    dma_channel_configure(dma_channel, &c,
        input_buffer,
        &pio->rxf[sm],
        (~0),
        true
    );

    return dma_channel;
}

int main()
{
    int input_sm, dma_channel;
    int i = 0;
    uint highest_time = ~0;

    memset(input_buffer, ~0, INPUT_BUFFER_SIZE * sizeof(uint32_t));

    stdio_init_all();

    printf("\n\n=== ssm_input_test ===\n");

    clk_sys_hz = clock_get_hz(clk_sys);

    printf("System clock is running at %u Hz\n", clk_sys_hz);
    printf("At a period of %.3fns\n", (2. / clk_sys_hz) * 1000000000.);

    gpio_set_pulls(PIN_IN, true, false); //Set to pull-up
    input_sm = ssm_input_program_start(pio0, PIN_IN, 32);

    printf("Enabled state machines\n");

    dma_channel = pio_dma_init(pio0, input_sm, input_buffer);

    while(true) {
        if (pio_interrupt_get(pio0, SSM_INPUT_IRQ_NUM)) {
            for (; input_buffer[i+1] < highest_time; i = (i + 2) % INPUT_BUFFER_SIZE) {
                printf("Input %lu at time %lu\n", input_buffer[i], (~0) - input_buffer[i+1]);
                highest_time = input_buffer[i+1];
            }
            printf("\n");
            pio_interrupt_clear(pio0, SSM_INPUT_IRQ_NUM);
        }
    }
}
