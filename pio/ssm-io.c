#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/pio.h>
#include <hardware/timer.h>
#include <stdlib.h>

#include "ssm-input.pio.h"
#include "ssm-output.pio.h"
#include "ssm-rp2040-internal.h"

#define SSM_PIO pio0
#define INPUT_SM 0

#define SSM_PIO_IRQ_ENABLE_SOURCE pio_set_irq0_source_enabled
#define INPUT_IRQ PIO0_IRQ_0
#define INPUT_IRQ_SOURCE PIO_INTR_SM0_LSB

#define PIO_RING_BUFFER_LOG2_SIZE 6
#define PIO_RING_BUFFER_SIZE (1 << PIO_RING_BUFFER_LOG2_SIZE)

// PIO input ring buffer; all its lower address bits should be 0
uint8_t pio_ring_buffer[PIO_RING_BUFFER_SIZE]
    __attribute((aligned(PIO_RING_BUFFER_SIZE)));

/*
uint32_t *pio_ring_buffer_ptr = (uint32_t *) pio_ring_buffer;
uint32_t *pio_ring_buffer_top =
(uint32_t *) (pio_ring_buffer + PIO_RING_BUFFER_SIZE);
*/

// State machines related to the output system
#define ALARM_SM 1
#define BUFFER_SM 2

// DMA channel for managing the ring buffer
int pio_input_dma_channel;
dma_channel_config pio_input_dma_config;

// The DMA channel's next write address
#define INPUT_DMA_WRITE_ADDR                                                   \
  ((uint32_t *)dma_hw->ch[pio_input_dma_channel].write_addr)

static void input_fifo_isr(void) {
  pio_interrupt_clear(SSM_PIO, SSM_INPUT_IRQ_NUM);
  sem_release(&ssm_tick_sem);
}

/*** DMA ***/

static inline void setup_dma(void) {
  pio_input_dma_channel = dma_claim_unused_channel(true);

  pio_input_dma_config = dma_channel_get_default_config(pio_input_dma_channel);

  channel_config_set_transfer_data_size(&pio_input_dma_config, DMA_SIZE_32);
  channel_config_set_read_increment(&pio_input_dma_config, false);
  channel_config_set_write_increment(&pio_input_dma_config, true);
  channel_config_set_dreq(&pio_input_dma_config,
                          pio_get_dreq(SSM_PIO, INPUT_SM, false));
  channel_config_set_ring(&pio_input_dma_config, true,
                          PIO_RING_BUFFER_LOG2_SIZE);

  dma_channel_configure(pio_input_dma_channel, &pio_input_dma_config,
                        pio_ring_buffer,         // Write to ring buffer
                        &SSM_PIO->rxf[INPUT_SM], // read from PIO RX FIFO
                        (~0), // Count: make it big. FIXME: restart
                        true);
}

/*** IRQ ***/

static inline void setup_irq(void) {
  irq_set_exclusive_handler(INPUT_IRQ, input_fifo_isr);
  SSM_PIO_IRQ_ENABLE_SOURCE(SSM_PIO, INPUT_IRQ_SOURCE, true);
  irq_set_enabled(INPUT_IRQ, true);
}

/*** PIO ***/

/* Left here for reference. Assumes input_count and output_count are non-zero
static inline void setup_pio(uint input_base, uint input_count,
                             uint output_base, uint output_count) {

  ssm_input_init(SSM_PIO, INPUT_SM, input_base, input_count);
  ssm_output_alarm_init(SSM_PIO, ALARM_SM);
  ssm_output_buffer_init(SSM_PIO, BUFFER_SM, output_base, output_count);

  uint32_t ctr_init = ~(timer_hw->timerawl << 4);

  pio_sm_put(SSM_PIO, INPUT_SM, ctr_init);
  pio_sm_put(SSM_PIO, ALARM_SM, ctr_init);

  pio_set_sm_mask_enabled(SSM_PIO,
                          1 << INPUT_SM | 1 << ALARM_SM | 1 << BUFFER_SM, true);
}
*/

/*** Entry point ***/

size_t input_vars_len = 0, output_vars_len = 0;
ssm_value_t *input_vars = NULL, *output_vars = NULL;

void ssm_rp2040_io_init(uint input_base, uint input_count, uint output_base,
                        uint output_count, ssm_value_t *input_out,
                        ssm_value_t *output_out) {

  // Ensure this function is only ever called once.
  static bool already_called = false;
  assert(!already_called);
  already_called = true;

  // Claim state machines
  pio_sm_claim(SSM_PIO, INPUT_SM);
  pio_sm_claim(SSM_PIO, ALARM_SM);
  pio_sm_claim(SSM_PIO, BUFFER_SM);

  uint sm_mask = 0;

  if (input_count > 0) {
    input_vars_len = input_count;
    input_vars = malloc(sizeof(ssm_value_t) * input_vars_len);

    // NOTE: reading the GPIO pin here like this is slightly prone to races
    if (input_count == 1) {
      *input_out = *input_vars = ssm_new_sv(ssm_marshal(gpio_get(input_base)));
    } else {
      *input_out = ssm_new_adt(input_count, 0);
      for (int i = 0; i < input_count; i++) {
        ssm_value_t sv = ssm_new_sv(ssm_marshal(gpio_get(input_base + i)));
        input_vars[i] = sv;
        ssm_adt_field(*input_out, i);
      }
    }

    setup_irq();
    setup_dma();
    ssm_input_init(SSM_PIO, INPUT_SM, input_base, input_count);
    sm_mask |= 1 << INPUT_SM;
  }

  if (output_count > 0) {
    output_vars_len = output_count;
    output_vars = malloc(sizeof(ssm_value_t) * output_vars_len);

    if (output_count == 1) {
      *output_out = *output_vars = ssm_new_sv(ssm_marshal(0));
    } else {
      *output_out = ssm_new_adt(output_count, 0);
      for (int i = 0; i < output_count; i++) {
        ssm_value_t sv = ssm_new_sv(ssm_marshal(0));
        output_vars[i] = sv;
        ssm_adt_field(*output_out, i);
      }
    }

    ssm_output_alarm_init(SSM_PIO, ALARM_SM);
    ssm_output_buffer_init(SSM_PIO, BUFFER_SM, output_base, output_count);

    sm_mask |= 1 << BUFFER_SM | 1 << ALARM_SM;
  }

  if (sm_mask) {
    // The next section has tight timing requirements, so we need to ensure the
    // compiler doesn't reorder anything past this point.
    __compiler_memory_barrier();

    uint32_t ctr_init = ~(timer_hw->timerawl << 4);

    pio_sm_put(SSM_PIO, INPUT_SM, ctr_init);
    pio_sm_put(SSM_PIO, ALARM_SM, ctr_init);

    pio_set_sm_mask_enabled(SSM_PIO, sm_mask, true);
  }
}

int ssm_rp2040_try_input(ssm_time_t next_time) {
  // FIXME: write this
  return 0;
}

void ssm_rp2040_forward_output(void) {
  if (!output_vars)
    return;

  for (int i = 0; i < output_vars_len; i++) {
    // FIXME: Stephen's technique does not work for multiple output vars
    // scheduled for different times!
  }
}

ssm_value_t rp2040_io_init(ssm_value_t input, ssm_value_t output) {
  // FIXME: write this. It should be callable from sslang programs.
  return ssm_marshal(0);
}
