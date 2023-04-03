/**
 * Override for ssm_throw to properly panic and log reason.
 */
// TODO: test that this actually works, i.e., is linked in

#include <ssm.h>
#include <stdlib.h>
#include <stdio.h>

/** Override ssm_throw function with some platform-specific logging. */
void ssm_throw(ssm_error_t reason, const char *file, int line, const char *func) {
  printf("Threw error code %d at time: %016llx", reason, ssm_now());
  switch (reason) {
  case SSM_INTERNAL_ERROR:
    printf("Unforeseen internal error.");
    break;
  case SSM_EXHAUSTED_ACT_QUEUE:
    printf("Tried to insert into full activation record queue.");
    break;
  case SSM_EXHAUSTED_EVENT_QUEUE:
    printf("Tried to insert into full event queue.");
    break;
  case SSM_EXHAUSTED_MEMORY:
    printf("Could not allocate more memory.");
    break;
  case SSM_EXHAUSTED_PRIORITY:
    printf("Tried to exceed available recursion depth.");
    break;
  case SSM_INVALID_TIME:
    printf(
        "Invalid time, e.g., scheduled delayed assignment at an earlier time.");
    break;
  default:
    printf("Unknown/platform-specific error.");
    break;
  }
  exit(reason);
}
