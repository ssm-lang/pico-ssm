#include "ssm-rp2040-internal.h"
#include <stdlib.h>

#ifndef SSM_NUM_PAGES
#define SSM_NUM_PAGES 32
#endif

static char mem[SSM_NUM_PAGES][SSM_MEM_PAGE_SIZE] = {0};
static size_t allocated_pages = 0;

static void *alloc_page(void) {
  if (allocated_pages >= SSM_NUM_PAGES)
    SSM_THROW(SSM_EXHAUSTED_MEMORY);
  return mem[allocated_pages++];
}

static void *alloc_mem(size_t size) { return malloc(size); }

static void free_mem(void *mem, size_t size) { free(mem); }

void ssm_rp2040_mem_init(void) {
  // Consider registering this as ctor
  ssm_mem_init(&alloc_page, &alloc_mem, &free_mem);
}
