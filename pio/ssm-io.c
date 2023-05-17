#include <hardware/gpio.h>
#include <hardware/pio.h>

#include "ssm-input.pio.h"
#include "ssm-output.pio.h"
#include "ssm-rp2040-internal.h"

#define SSM_PIO pio0
#define INPUT_SM 0

#define SSM_PIO_IRQ_ENABLE_SOURCE pio_set_irq0_source_enabled
#define INPUT_IRQ PIO0_IRQ_0
#define INPUT_IRQ_SOURCE PIO_INTR_SM0_LSB

// State machines related to the output system
#define ALARM_SM 1
#define BUFFER_SM 2

void ssm_rp2040_io_init(uint input_base, uint input_count, uint output_base,
                        uint output_count) {
  // Claim state machines
  pio_sm_claim(SSM_PIO, INPUT_SM);
  pio_sm_claim(SSM_PIO, ALARM_SM);
  pio_sm_claim(SSM_PIO, BUFFER_SM);

  ssm_input_init(SSM_PIO, INPUT_SM, input_base, input_count);
  ssm_output_alarm_init(SSM_PIO, ALARM_SM);
  ssm_output_buffer_init(SSM_PIO, BUFFER_SM, output_base, output_count);
}
