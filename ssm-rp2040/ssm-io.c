#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/pio.h>
#include <hardware/timer.h>

#include "ssm-input.pio.h"
#include "ssm-output.pio.h"
#include "ssm-rp2040-internal.h"

// In 16MHz
#define CTR_INIT_SKEW_OFFSET 32

#define SSM_PIO pio0
#define INPUT_SM 0

#define SSM_PIO_IRQ_ENABLE_SOURCE pio_set_irq0_source_enabled
#define INPUT_IRQ PIO0_IRQ_0
#define INPUT_IRQ_SOURCE PIO_INTR_SM0_LSB

#define PIO_RING_BUFFER_LOG2_SIZE 8
#define PIO_RING_BUFFER_SIZE (1 << PIO_RING_BUFFER_LOG2_SIZE)

// How if scheduled output is within this margin, it is considered too late
// Units: ssm_time_t
#define PIO_SCHED_OUTPUT_MARGIN 32

// PIO input ring buffer; all its lower address bits should be 0
uint8_t pio_ring_buffer[PIO_RING_BUFFER_SIZE]
    __attribute((aligned(PIO_RING_BUFFER_SIZE)));

// State machines related to the output system
#define ALARM_SM 1
#define BUFFER_SM 2

// DMA channel for managing the ring buffer
int pio_input_dma_channel;
int pio_input_control_dma_channel;
dma_channel_config pio_input_dma_config;
dma_channel_config pio_input_control_dma_config;

static void input_fifo_isr(void) {
  pio_interrupt_clear(SSM_PIO, SSM_INPUT_IRQ_NUM);
  sem_release(&ssm_tick_sem);
}

/*** DMA ***/

static inline void setup_dma(void) {
  pio_input_dma_channel = dma_claim_unused_channel(true);
  pio_input_dma_config = dma_channel_get_default_config(pio_input_dma_channel);

  pio_input_control_dma_channel = dma_claim_unused_channel(true);
  pio_input_control_dma_config =
      dma_channel_get_default_config(pio_input_control_dma_channel);

  channel_config_set_transfer_data_size(&pio_input_dma_config, DMA_SIZE_32);
  channel_config_set_read_increment(&pio_input_dma_config, false);
  channel_config_set_write_increment(&pio_input_dma_config, true);
  channel_config_set_dreq(&pio_input_dma_config,
                          pio_get_dreq(SSM_PIO, INPUT_SM, false));
  channel_config_set_ring(&pio_input_dma_config, true,
                          PIO_RING_BUFFER_LOG2_SIZE);
  channel_config_set_chain_to(&pio_input_dma_config,
                              pio_input_control_dma_channel);

  dma_channel_configure(pio_input_dma_channel, &pio_input_dma_config,
                        pio_ring_buffer,         // Write to ring buffer
                        &SSM_PIO->rxf[INPUT_SM], // read from PIO RX FIFO
                        (~0),                    // Count: make it big.
                        false);

  channel_config_set_transfer_data_size(&pio_input_control_dma_config,
                                        DMA_SIZE_32);
  channel_config_set_read_increment(&pio_input_control_dma_config, false);
  channel_config_set_write_increment(&pio_input_control_dma_config, false);

  dma_channel_configure(
      pio_input_control_dma_channel, &pio_input_control_dma_config,
      // Write to the write address, but also trigger
      &dma_hw->ch[pio_input_dma_channel].al2_write_addr_trig,
      // Read from the write address
      &dma_hw->ch[pio_input_dma_channel].write_addr, 1, false);

  dma_channel_start(pio_input_dma_channel);
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

ssm_value_t gpio_input_var = ssm_marshal(0), gpio_output_var = ssm_marshal(0);

void ssm_rp2040_io_init(uint input_base, uint input_count, uint output_base,
                        uint output_count, ssm_value_t *input_out,
                        ssm_value_t *output_out) {

  // Ensure this function is only ever called once.
  static bool already_called = false;
  assert(!already_called);
  already_called = true;

  valid_params_if(PIO, input_count == 0 || input_base + input_count <= NUM_BANK0_GPIOS);
  valid_params_if(PIO, output_count == 0 || output_base + output_count <= NUM_BANK0_GPIOS);

  // Claim state machines
  pio_sm_claim(SSM_PIO, INPUT_SM);
  pio_sm_claim(SSM_PIO, ALARM_SM);
  pio_sm_claim(SSM_PIO, BUFFER_SM);

  uint sm_mask = 0;

  *input_out = ssm_new_sv(ssm_marshal(0));
  *output_out = ssm_new_sv(ssm_marshal(0));

  if (input_count > 0) {
    gpio_input_var = *input_out;
    ssm_dup(gpio_input_var);

    uint32_t gpio_init = (gpio_get_all() >> input_base) & ((1u << input_count) - 1);
    ssm_to_sv(gpio_input_var)->value = ssm_marshal(gpio_init);

    setup_irq();
    setup_dma();
    ssm_input_init(SSM_PIO, INPUT_SM, input_base, input_count);
    sm_mask |= 1 << INPUT_SM;
  }

  if (output_count > 0) {
    gpio_output_var = *output_out;
    ssm_dup(gpio_output_var);

    ssm_output_alarm_init(SSM_PIO, ALARM_SM);
    ssm_output_buffer_init(SSM_PIO, BUFFER_SM, output_base, output_count);
    sm_mask |= 1 << BUFFER_SM | 1 << ALARM_SM;
  }

  if (sm_mask) {
    // The next section has tight timing requirements, so we need to ensure the
    // compiler doesn't reorder anything past this point.
    __compiler_memory_barrier();

    uint32_t ctr_init = ~((timer_hw->timerawl << 4) + CTR_INIT_SKEW_OFFSET);

    pio_sm_put(SSM_PIO, INPUT_SM, ctr_init);
    pio_sm_put(SSM_PIO, ALARM_SM, ctr_init);

    pio_set_sm_mask_enabled(SSM_PIO, sm_mask, true);
  }
}

typedef struct pio_input_packet {
  uint32_t val;
  uint32_t ctr;
} pio_input_t;

pio_input_t *pio_rb_cur = (pio_input_t *)pio_ring_buffer;

const pio_input_t *pio_rb_top =
    (pio_input_t *)(pio_ring_buffer + PIO_RING_BUFFER_SIZE);

// The DMA channel's next write address
#define INPUT_DMA_WRITE_ADDR                                                   \
  ((pio_input_t *)dma_hw->ch[pio_input_dma_channel].write_addr)

int ssm_rp2040_try_input(ssm_time_t next_time) {
  if (!ssm_on_heap(gpio_input_var))
    // Input was never initialized, we are not using input
    return 0;

  if (pio_rb_cur == INPUT_DMA_WRITE_ADDR)
    // No new input
    return 0;

  uint32_t val = pio_rb_cur[0].val, ctr = pio_rb_cur[0].ctr;

  // TODO: this is a rather expensive check because it involves reading the
  // timer. We should be able to avoid it unless there's wraparound.
  ssm_time_t input_time = ctr_to_time(ctr);

  if (input_time > next_time)
    // Still catching up to real-time, not ready for input yet; keep it buffered
    return 0;

  // Advance pio_rb_cur
  if (++pio_rb_cur >= pio_rb_top)
    // Wrap it around
    pio_rb_cur = (pio_input_t *)pio_ring_buffer;

  // Schedule the event
  ssm_sv_later_unsafe(ssm_to_sv(gpio_input_var), input_time, ssm_marshal(val));

  return 1;
}

void ssm_pio_schedule_output(pio_ctr_t t, uint32_t v) {
  pio_sm_put(SSM_PIO, BUFFER_SM, v);
  __compiler_memory_barrier();

  pio_sm_exec(SSM_PIO, BUFFER_SM,
              pio_encode_pull(false, true) /* pull block */);

  pio_sm_put(SSM_PIO, ALARM_SM, t);
}

void ssm_pio_force_output(uint32_t v) {
  pio_sm_put(SSM_PIO, BUFFER_SM, v);

  __compiler_memory_barrier();

  pio_sm_exec(SSM_PIO, BUFFER_SM,
              pio_encode_pull(false, true) /* pull block */);

  __compiler_memory_barrier();

  pio_sm_exec(SSM_PIO, BUFFER_SM,
              pio_encode_out(pio_pins, 32) /* out pins, 32 */);
}

void ssm_rp2040_forward_output(void) {
  if (!ssm_on_heap(gpio_output_var))
    // Output was never initialized, we are not using output
    return;

  ssm_time_t later_time = ssm_to_sv(gpio_output_var)->later_time;
  if (later_time != SSM_NEVER) {
    uint32_t ctr = time_to_ctr(later_time),
             val = ssm_unmarshal(ssm_to_sv(gpio_output_var)->later_value);

    ssm_time_t cur_time = get_real_time();
    if (cur_time + PIO_SCHED_OUTPUT_MARGIN < later_time)
      // Schedule output
      ssm_pio_schedule_output(ctr, val);
    else
      // Too late to schedule, output it as soon as possible (or drop?)
      ssm_pio_force_output(val);
  }
}

/*** sslang config ***/

ssm_value_t rp2040_io_init(ssm_value_t input, ssm_value_t output) {
  // extern rp2040_io_init : (Int, Int) -> (Int, Int) -> (&Int, &Int)

  uint input_base = ssm_unmarshal(ssm_adt_field(input, 0)),
       input_count = ssm_unmarshal(ssm_adt_field(input, 1)),
       output_base = ssm_unmarshal(ssm_adt_field(output, 0)),
       output_count = ssm_unmarshal(ssm_adt_field(output, 1));

  ssm_drop(input);
  ssm_drop(output);

  ssm_value_t input_var, output_var;

  ssm_rp2040_io_init(input_base, input_count, output_base, output_count,
                     &input_var, &output_var);

  ssm_value_t ret = ssm_new_adt(2, 0);
  ssm_adt_field(ret, 0) = input_var;
  ssm_adt_field(ret, 1) = output_var;

  return ret;
}
