#ifndef SSM_RP2040_INTERNAL_H
#define SSM_RP2040_INTERNAL_H

#include <ssm-internal.h>

typedef unsigned int uint;

void ssm_rp2040_mem_init(void);
void ssm_rp2040_io_init(uint input_base, uint input_count, uint output_base,
                        uint output_count);

#endif /* ifndef SSM_RP2040_INTERNAL_H */
