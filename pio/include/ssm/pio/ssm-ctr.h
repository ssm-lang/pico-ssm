#ifndef SSM_CTR_H
#define SSM_CTR_H

// Terminology:
// ticks       := unsigned 32-/64-bit that counts up, read from system clock
// counter/ctr := unsigned 32-bit integer that counts down once every 8 ticks

#include <stdint.h>

static inline uint32_t ctr_to_ticks(uint32_t ctr) { return ~ctr * 8; }
static inline uint32_t ticks_to_ctr(uint32_t ticks) { return ~(ticks / 8); }

#endif /* ifndef SSM_CTR_H */
