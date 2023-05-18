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

#define INPUT_BUFFER_BITS 8
#define INPUT_BUFFER_BYTES (1 << INPUT_BUFFER_BITS)
#define PIN_IN 22
#define PIN_OUT PICO_DEFAULT_LED_PIN
#define START_TIME 0

#define DMA_CHANNEL_MAIN 0
#define DMA_CHANNEL_CONTROL 1

uint8_t input_buffer[INPUT_BUFFER_BYTES] __attribute__((aligned(INPUT_BUFFER_BYTES)));
uint32_t clk_sys_hz = 125000000; // Default is (usually) 125MHz (?)

void pio_dma_init(PIO pio, uint sm, int dma_channels[2]) {

    dma_channels[DMA_CHANNEL_MAIN] = dma_claim_unused_channel(true);
    dma_channels[DMA_CHANNEL_CONTROL] = dma_claim_unused_channel(true);

    dma_channel_config c = dma_channel_get_default_config(dma_channels[DMA_CHANNEL_MAIN]);
    //Read a word at a time
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    //Reads are always from the FIFO, don't increment
    channel_config_set_read_increment(&c, false);
    //Writes should increment
    channel_config_set_write_increment(&c, true);
    //Get transfers from the RX FIFO
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));
    //Make writes go in a ring
    channel_config_set_ring(&c, true, INPUT_BUFFER_BITS);
    //Make the channel chain to the other channel
    channel_config_set_chain_to(&c, dma_channels[DMA_CHANNEL_CONTROL]);

    dma_channel_configure(dma_channels[DMA_CHANNEL_MAIN], &c,
        input_buffer,
        &pio->rxf[sm],
        (~0),
        false
    );
    
    //Configure the control channel
    c = dma_channel_get_default_config(dma_channels[DMA_CHANNEL_CONTROL]);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    //We're only writing to one place, so don't increment either read or write
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, false);

    //Write the write address back to the main DMA channel's write address
    dma_channel_configure(dma_channels[DMA_CHANNEL_CONTROL], &c,
        &dma_hw->ch[dma_channels[DMA_CHANNEL_MAIN]].al2_write_addr_trig,
        &dma_hw->ch[dma_channels[DMA_CHANNEL_MAIN]].write_addr,
        1,
        false
    );
}

void handle_inputs(uint32_t input, uint32_t time){
    printf("Input %lu at time %lu\n", input, (~0) - time);
}

int main()
{
    int input_sm;
    int dma_channels[2];
    int i = 0;
    uint32_t *input_buffer_uint = (uint32_t *)input_buffer;
    io_rw_32 start_write_addr = (io_rw_32)input_buffer;

    memset(input_buffer, ~0, INPUT_BUFFER_BYTES);

    stdio_init_all();

    printf("\n\n=== ssm_input_test ===\n");

    clk_sys_hz = clock_get_hz(clk_sys);

    printf("System clock is running at %u Hz\n", clk_sys_hz);
    printf("At a period of %.3fns\n", (2. / clk_sys_hz) * 1000000000.);

    gpio_set_pulls(PIN_IN, true, false); //Set to pull-up
    input_sm = ssm_input_program_start(pio0, PIN_IN, 1);

    printf("Enabled state machines\n");

    pio_dma_init(pio0, input_sm, dma_channels);

    dma_channel_start(dma_channels[DMA_CHANNEL_MAIN]);
    pio_sm_put(pio0, input_sm, START_TIME);

    while(true) {
        if (pio_interrupt_get(pio0, SSM_INPUT_IRQ_NUM)) {
            for (; start_write_addr + i * sizeof(uint32_t) != dma_channel_hw_addr(dma_channels[DMA_CHANNEL_MAIN])->write_addr; i = (i + 2) % (INPUT_BUFFER_BYTES / sizeof(uint32_t)))
                handle_inputs(input_buffer_uint[i], input_buffer_uint[i+1]);
            printf("\n");
            pio_interrupt_clear(pio0, SSM_INPUT_IRQ_NUM);
        }
    }
}
