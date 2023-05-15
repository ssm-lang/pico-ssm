#include <stdio.h>
#include <pico/sem.h>
#include <pico/time.h>
#include <hardware/timer.h>
#include <hardware/irq.h>
// Requires hardware_pio among the target_link_libraries
#include "hardware/pio.h"
// Requires hardware_dma among the larget_link_libraries 
#include "hardware/dma.h"

#include "ssm-gpio.pio.h"

#include <ssm-internal.h>
#include <ssm-platform.h>

#define INPUT_PIN_BASE 4
#define INPUT_PIN_COUNT 2
#define OUTPUT_PIN_BASE 14
#define OUTPUT_PIN_COUNT 1

/******************************
 * Memory allocation
 ******************************/

#ifndef SSM_NUM_PAGES
#  define SSM_NUM_PAGES 32
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

/******************************
 * Exception Handling
 ******************************/

// The order of these needs to match up with the codes in enum ssm_error in ssm.h
static const char *ssm_exception_message[] = {
    "Unforeseen internal error",
    "Tried to insert into full activation record queue",
    "Tried to insert into full event queue",
    "Could not allocate more memory",
    "Tried to exceed available recursion depth",
    "Invalid time, e.g., scheduled delayed assignment at an earlier time",
    "Not yet ready to perform the requested action",
    "Specified invalid time, e.g., scheduled assignment at an earlier time",
    "Invalid memory layout, e.g., using a pointer where int was expected"
}; 

void ssm_throw(ssm_error_t reason, const char *file, int line, const char *func) {
  printf("Threw error code %d at time: %016llx", reason, ssm_now());
  printf("%s\n", ssm_exception_message[reason < SSM_PLATFORM_ERROR ?
				       reason : 0]);
  exit(reason);
}

#define ALARM_IRQ TIMER_IRQ_0
#define ALARM_NUM 0

/******************************
 * Semaphore and timer interrupt
 ******************************/

semaphore_t ssm_tick_sem;

static void timer_isr(void) {
  // Compulsory: clear the interrupt
  hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
  sem_release(&ssm_tick_sem); // Wake up the tick loop
}

/******************************
 * Low-level 64-bit timer access
 *
 * See the rp2040 datasheet, page 557
 *****************************/

/* Read the 64-bit time value
 *
 * Potentially unsafe if the other core is simultaneously trying
 * to read the timer at the same time
 */
ssm_time_t get_real_time(void) {
  uint32_t lo = timer_hw->timelr;
  uint32_t hi = timer_hw->timehr;
  return ( (ssm_time_t) hi << 32u ) | lo;
}

/* Set the alarm time
 *
 * Note that this is only a 32-bit comparator so strange things
 * might happen with long delays
 */
void set_alarm(ssm_time_t when)
{
  timer_hw->alarm[ALARM_NUM] = (uint32_t) when;
}

/*
 * Initialize our alarm
 */
void initialize_alarm()
{
  hardware_alarm_claim(ALARM_NUM);
  
  hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
  irq_set_exclusive_handler(ALARM_IRQ, timer_isr);
  irq_set_enabled(ALARM_IRQ, true);
  set_alarm((ssm_time_t) ~0);   // Far enough in the future to be irrelevant
}

/******************************
 * PIO Input/Output system
 ******************************/

// PIO, state machine, and interrupts related to the input system
// The PIO, SM, and IRQ numbers must be consistent
#define INPUT_PIO pio0
#define INPUT_SM 0
#define INPUT_PIO_IRQ_ENABLE_SOURCE pio_set_irq0_source_enabled
#define INPUT_IRQ PIO0_IRQ_0
#define INPUT_IRQ_SOURCE PIO_INTR_SM0_LSB

// PIO and state machine related to the output system
#define OUTPUT_PIO pio1
#define ALARM_SM 0
#define GPIO_SM 1

// Convert the 1us hardware counter to PIO counter values (16 MHz)
#define US_TO_PIO(t) (~((uint32_t) ((t) << 4)))
#define PIO_TO_US(t) ((uint64_t) (~(t) >> 4))

// Number of address bits in the byte address; max 14
#define PIO_RING_BUFFER_LOG2_SIZE 6
#define PIO_RING_BUFFER_SIZE (1 << PIO_RING_BUFFER_LOG2_SIZE)

// PIO input ring buffer; all its lower address bits should be 0
uint8_t pio_ring_buffer[PIO_RING_BUFFER_SIZE]
          __attribute((aligned(PIO_RING_BUFFER_SIZE)));

uint32_t *pio_ring_buffer_ptr = (uint32_t *) pio_ring_buffer;
uint32_t *pio_ring_buffer_top =
  (uint32_t *) (pio_ring_buffer + PIO_RING_BUFFER_SIZE);

// DMA channel for managing the ring buffer
int pio_input_dma_channel;

static void input_fifo_isr(void) {
  pio_interrupt_clear(INPUT_PIO, SSM_INPUT_IRQ_NUM);
  // printf(".");
  sem_release(&ssm_tick_sem); // Wake up the tick loop
}

static inline void
ssm_pio_gpio_init(uint input_pins_base, uint input_pins_count,
		  uint output_pins_base, uint output_pins_count)
{
  // Configure the input pins to be read by the PIO
  for (uint i = 0 ; i < input_pins_count ; i++)
    pio_gpio_init(INPUT_PIO, input_pins_base + i); // FIXME: necessary?
  pio_sm_set_consecutive_pindirs(INPUT_PIO, INPUT_SM,
				 input_pins_base, input_pins_count, false);
  
  // Configure the output pins to be driven by the PIO
  for (uint i = 0 ; i < output_pins_count ; i++)
    pio_gpio_init(OUTPUT_PIO, output_pins_base + i);
  pio_sm_set_consecutive_pindirs(OUTPUT_PIO, GPIO_SM,
				 output_pins_base, output_pins_count, true);

  // Upload the input program
  pio_sm_claim(INPUT_PIO, INPUT_SM);
  uint input_offset = pio_add_program(INPUT_PIO, &ssm_input_program);
  
  pio_sm_config input_c = ssm_input_program_get_default_config(input_offset); 
  sm_config_set_in_pins(&input_c, input_pins_base);
  pio_sm_init(INPUT_PIO, INPUT_SM, input_offset, &input_c);

  // Patch the input program based on the number of input pins
  
  uint null_count = 32 - input_pins_count;
  uint pin_in_instr = pio_encode_in(pio_pins, input_pins_count);
  uint null_in_instr = (null_count > 0) ? pio_encode_in(pio_null, null_count) :
                                          pio_encode_nop();

  INPUT_PIO->instr_mem[input_offset + ssm_input_PINS_PATCH_0] = pin_in_instr;
  INPUT_PIO->instr_mem[input_offset + ssm_input_PINS_PATCH_0 + 1] = null_in_instr;
  INPUT_PIO->instr_mem[input_offset + ssm_input_PINS_PATCH_1] = pin_in_instr;
  INPUT_PIO->instr_mem[input_offset + ssm_input_PINS_PATCH_1 + 1] = null_in_instr;

 
  // Enable the PIO input interrupt
  //
  // This is tricky: IRQ numbering differs from PIOland to
  // processorland, and the function to enable a PIO IRQ depends
  // on the IRQ in question
  irq_set_exclusive_handler(INPUT_IRQ, input_fifo_isr);
  INPUT_PIO_IRQ_ENABLE_SOURCE(INPUT_PIO, INPUT_IRQ_SOURCE, true);
  irq_set_enabled(INPUT_IRQ, true);

  // Initialize the DMA channel that will transfer data from
  // the input program into a ring buffer

  pio_input_dma_channel = dma_claim_unused_channel(true);
    
  dma_channel_config c = dma_channel_get_default_config(pio_input_dma_channel);
  // FIXME: verify one was acquired

  channel_config_set_transfer_data_size(&c, DMA_SIZE_32); // 32 bit words 
  channel_config_set_read_increment(&c, false); // Don't increment FIFO reads
  channel_config_set_write_increment(&c, true); // Increment buffer writes
  channel_config_set_dreq(&c,
       pio_get_dreq(INPUT_PIO, INPUT_SM, false)); // Rate from RX FIFO
  channel_config_set_ring(&c, true, PIO_RING_BUFFER_LOG2_SIZE); // Ring buffer

  dma_channel_configure(pio_input_dma_channel,
			&c,
			pio_ring_buffer, // Write to ring buffer
			&INPUT_PIO->rxf[INPUT_SM], // read from PIO RX FIFO
			(~0), // Count: make it big. FIXME: restart
			true);
  
  // FIXME: use two DMA channels: one for the ring buffer data,
  // the other for restarting the first, and chain the two together
  // The second should simply load the same large count to the first



  
  
  // Upload the output alarm program
  pio_sm_claim(OUTPUT_PIO, ALARM_SM);
  uint alarm_offset = pio_add_program(OUTPUT_PIO, &ssm_output_alarm_program);
  
  pio_sm_config alarm_c =
    ssm_output_alarm_program_get_default_config(alarm_offset);
  // FIXME: more configuration stuff?
  pio_sm_init(OUTPUT_PIO, ALARM_SM, alarm_offset, &alarm_c);

  // Upload the gpio output program
  pio_sm_claim(OUTPUT_PIO, GPIO_SM);
  uint gpio_offset = pio_add_program(OUTPUT_PIO, &ssm_output_gpio_program);
  
  pio_sm_config gpio_c =
    ssm_output_gpio_program_get_default_config(gpio_offset);
  sm_config_set_out_pins(&gpio_c, output_pins_base, output_pins_count);
  pio_sm_init(OUTPUT_PIO, GPIO_SM, gpio_offset, &gpio_c);


  

  // Initialize the input and output timers
  // based on the current hardware counter value
  
  // FIXME: We'd like this to be even more precise since we don't know
  // the exact latency between reading the hardware counter
  // and starting the state machines
  
  uint32_t pio_count = US_TO_PIO(timer_hw->timelr);
  pio_sm_put(OUTPUT_PIO, ALARM_SM, pio_count);
  pio_sm_put(INPUT_PIO, INPUT_SM, pio_count);

  pio_set_sm_mask_enabled(OUTPUT_PIO, (1 << ALARM_SM) | (1 << GPIO_SM), true);
  pio_set_sm_mask_enabled(INPUT_PIO, 1 << INPUT_SM, true);  
}

static inline void
ssm_pio_schedule_output(ssm_time_t pio_time, uint32_t value) {
  // Send the new output value to the GPIO state machine
  pio_sm_put(OUTPUT_PIO, GPIO_SM, value);  // put value into TX FIFO
  __compiler_memory_barrier();
  // execute pull block, i.e., OSR <- TX FIFO
  pio_sm_exec(OUTPUT_PIO, GPIO_SM, pio_encode_pull(false, true));

  // Set the new alarm time
  pio_sm_put(OUTPUT_PIO, ALARM_SM, US_TO_PIO(pio_time));
}



/******************************
 * Main tick loop
 ******************************/

int ssm_platform_entry(void) {

  // FIXME: Set the button pin to be a pull-up
  gpio_set_pulls(INPUT_PIN_BASE, true, false);

  // Initialize the semaphore
  sem_init(&ssm_tick_sem, 0, 1);
  
  // Set up the PIO input/output system; initialize the semaphore first
  ssm_pio_gpio_init(INPUT_PIN_BASE, INPUT_PIN_COUNT,
		    OUTPUT_PIN_BASE, OUTPUT_PIN_COUNT);

  // Configure our alarm
  initialize_alarm();
  
  ssm_mem_init(&alloc_page, &alloc_mem, &free_mem);

  // Prepare the sslang program to start
  extern ssm_act_t *__enter_main(ssm_act_t *, ssm_priority_t, ssm_depth_t,
                                 ssm_value_t *, ssm_value_t *);
  ssm_value_t nothing0 = ssm_new_sv(ssm_marshal(0));
  ssm_value_t gpio_output = ssm_new_sv(ssm_marshal(0));
  ssm_value_t main_argv[2] = {nothing0, gpio_output};
  
  ssm_activate(__enter_main(&ssm_top_parent, SSM_ROOT_PRIORITY, SSM_ROOT_DEPTH,
                            main_argv, NULL));

  // Run the program for time zero
  ssm_tick();

  // Run the program or sleep if we're too early
  for (;;) {
    ssm_time_t real_time = get_real_time();
    ssm_time_t next_time = ssm_next_event_time();

    printf("real %llu next %llu\n", real_time, next_time);

    while ( pio_ring_buffer_ptr != (uint32_t *) dma_hw->ch[pio_input_dma_channel].write_addr) {
      printf("in @ %llu : %8lx\n", PIO_TO_US(pio_ring_buffer_ptr[1]), pio_ring_buffer_ptr[0]);
      if (pio_ring_buffer_top == (pio_ring_buffer_ptr += 2))
	pio_ring_buffer_ptr = (uint32_t *) pio_ring_buffer;
    }

    // printf("DMA %lx\n", (uint32_t) dma_hw->ch[pio_input_dma_channel].write_addr - (uint32_t) pio_ring_buffer);

    __compiler_memory_barrier();

    if (next_time <= real_time) {
      ssm_tick(); // We're behind: catch up to reality

      // Update the scheduled GPIO
      ssm_time_t gpio_later = ssm_to_sv(gpio_output)->later_time;
      if (gpio_later != SSM_NEVER)	
	ssm_pio_schedule_output(gpio_later,
				ssm_unmarshal(ssm_to_sv(gpio_output)->later_value));

    } else if (next_time == SSM_NEVER)
      sem_acquire_blocking(&ssm_tick_sem); // Sleep until an external event
    else {
      set_alarm(next_time);                // Set the alarm to wake us up
      sem_acquire_blocking(&ssm_tick_sem); // Sleep until the alarm or earlier
      sem_reset(&ssm_tick_sem, 0);
    }    
  }
  
  return 0;
}

