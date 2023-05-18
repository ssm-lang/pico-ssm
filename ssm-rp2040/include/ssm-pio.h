#ifndef SSM_PIO_H
#define SSM_PIO_H

#include <stdint.h>

int ssm_pio_init(uint32_t output_mask);
void ssm_pio_schedule_output(uint64_t time_ns, uint32_t value_mask);

#endif /* ifndef SSM_PIO_H */
