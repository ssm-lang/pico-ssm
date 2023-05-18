#include <hardware/irq.h>

#include "ssm-rp2040-internal.h"

#define ALARM_IRQ TIMER_IRQ_0

static void timer_isr(void) {
  hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
  sem_release(&ssm_tick_sem);
}

void ssm_rp2040_alarm_init(void) {
  hardware_alarm_claim(ALARM_NUM);

  hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
  irq_set_exclusive_handler(ALARM_IRQ, timer_isr);
  irq_set_enabled(ALARM_IRQ, true);
}
