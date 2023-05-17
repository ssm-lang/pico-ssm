#include <pico/time.h>
#include <ssm-internal.h>
#include <ssm-platform.h>
#include <stdio.h>

/******************************
 * Exception Handling
 ******************************/

// The order of these needs to match with the codes in enum ssm_error in ssm.h
static const char *ssm_exception_message[] = {
    "Unforeseen internal error",
    "Tried to insert into full activation record queue",
    "Tried to insert into full event queue",
    "Could not allocate more memory",
    "Tried to exceed available recursion depth",
    "Invalid time, e.g., scheduled delayed assignment at an earlier time",
    "Not yet ready to perform the requested action",
    "Specified invalid time, e.g., scheduled assignment before \"now\"",
    "Invalid memory layout, e.g., using a pointer where int was expected",
};

void ssm_throw(ssm_error_t reason, const char *file, int line,
               const char *func) {
  printf("Threw error code %d at time: %llu\n", reason, ssm_now());
  printf("%s:%s:%d\n", file, func, line);
  printf("%s\n",
         ssm_exception_message[(reason < SSM_PLATFORM_ERROR) ? reason : 0]);
  sleep_ms(1000); // Allow the full error message to print
  exit(reason);
}
