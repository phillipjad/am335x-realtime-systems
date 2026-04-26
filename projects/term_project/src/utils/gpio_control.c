#include "gpio_control.h"

#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "logger.h"
#include "project_types.h"

/* GPIO Count */
#define GPIO_COUNT (4)
#define REGISTERS_PER_GROUP (32)
/* GPIO mmap constants */
#define PAGE_SIZE (4096U)
/* Base physical address for a GPIO modules (from TRM memory map) */
#define GPIO0_BASE_PHYS (0x44E07000)
#define GPIO1_BASE_PHYS (0x4804C000)
#define GPIO2_BASE_PHYS (0x481AC000)
#define GPIO3_BASE_PHYS (0x481AE000)
/* Offsets for registers within that GPIO module (from TRM register map) */
/* Output Enable - GPIO_OE Register - offset = 134h */
#define GPIO_OE_OFFSET (0x134)
/* Set Data Out - GPIO_SETDATAOUT Register - offset = 194h */
#define GPIO_SETDATAOUT_OFF (0x194)
/* Set Data In - GPIO_SETDATAIN Register - offset = 138h */
#define GPIO_DATAIN_OFFSET (0x138)
/* Clear Data Out - GPIO_CLEARDATAOUT Register - offset = 190h */
#define GPIO_CLEARDATAOUT_OFF (0x190)
// Store GPIOs
static gpio_map_t gpios_array[GPIO_COUNT];

// Helper to access 32-bit registers by offset
static inline volatile uint32_t *reg32(volatile uint8_t *base, uint32_t off) {
	return (volatile uint32_t *)((uintptr_t)base + off);
}

/*---------------------------------------------
 * Function: get_gpio_base - helper function
 *---------------------------------------------*/
static volatile uint8_t *get_gpio_base(uint8_t pin) {
	uint8_t gpio_base_nubmer = pin / REGISTERS_PER_GROUP;
	if (gpio_base_nubmer >= GPIO_COUNT || gpios_array[gpio_base_nubmer].gpio_base == NULL ||
	    gpios_array[gpio_base_nubmer].fd <= 0) {
		LOG_AND_EXIT("Failed to get gpio base for pin %d", pin);
		return NULL;
	} else {
		return gpios_array[gpio_base_nubmer].gpio_base;
	}
}

/*--------------------------------------
 * Function: gpio_map_init
 *--------------------------------------*/
void gpio_map_init(void) {
	// Page base
	uint32_t page_base = 0;
	// Page offset
	uint32_t page_off = 0;
	// Clear out
	(void)memset(gpios_array, 0, sizeof(gpios_array));

	// GPIOs
	const uint32_t addresses[GPIO_COUNT] = { GPIO0_BASE_PHYS, GPIO1_BASE_PHYS, GPIO2_BASE_PHYS, GPIO3_BASE_PHYS };

	// GPIO Setup
	for (int32_t ii = 0; ii < GPIO_COUNT; ++ii) {
		gpios_array[ii].fd = open("/dev/mem", O_RDWR | O_SYNC);
		if (gpios_array[ii].fd < 0) {
			LOG_AND_EXIT("Failed to open /dev/mem for GPIO PHYSICAL BASE: %d", ii);
		}

		page_base = (uint32_t)(addresses[ii] & ~(PAGE_SIZE - 1U));
		page_off = (uint32_t)(addresses[ii] - page_base);

		gpios_array[ii].map_base =
		    (volatile uint8_t *)mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gpios_array[ii].fd, page_base);
		if (gpios_array[ii].map_base == MAP_FAILED) {
			LOG_AND_EXIT("Failed to create mmap base for GPIO PHYSICAL BASE: %d, at physical address: 0x%X", ii, addresses[ii]);
		}

		gpios_array[ii].gpio_base = gpios_array[ii].map_base + page_off;
	}
}

/*--------------------------------------
 * Function: gpio_map_close
 *--------------------------------------*/
void gpio_map_close(void) {
	for (int32_t ii = 0; ii < GPIO_COUNT; ++ii) {
		if (gpios_array[ii].map_base && gpios_array[ii].map_base != MAP_FAILED) {
			munmap((void *)(uintptr_t)gpios_array[ii].map_base, PAGE_SIZE);
		}
		if (gpios_array[ii].fd >= 0) {
			close(gpios_array[ii].fd);
		}
	}
}

/*--------------------------------------
 * Function: gpio_set
 *--------------------------------------*/
void gpio_set(uint8_t pin, bool value) {
	// Get Base Address - exits if function fails
	volatile uint8_t *base = get_gpio_base(pin);

	// Calculate register number under GPIO Group
	uint8_t pin_number = pin % REGISTERS_PER_GROUP;

	// Depending on bool value can put register to 1 using set or 0 using clear
	// Get registers values
	volatile uint32_t *register_address = reg32(base, value ? GPIO_SETDATAOUT_OFF : GPIO_CLEARDATAOUT_OFF);
	// Set register value to 1
	*register_address = (1U << pin_number);
}

/*--------------------------------------
 * Function: gpio_read
 *--------------------------------------*/
uint8_t gpio_read(uint8_t pin) {
	// Get Base Address - exits if function fails
	volatile uint8_t *base = get_gpio_base(pin);

	// Calculate register number under GPIO Group
	uint8_t pin_number = pin % REGISTERS_PER_GROUP;

	// Get registers values
	volatile uint32_t *register_address = reg32(base, GPIO_DATAIN_OFFSET);
	// Read if register value is populated after bitmask, if so then return true
	return (*register_address & (1U << pin_number)) ? 1U : 0U;
}

/*--------------------------------------
 * Function: gpio_clear
 *--------------------------------------*/
void gpio_clear(uint8_t pin) {
	// Get Base Address - exits if function fails
	volatile uint8_t *base = get_gpio_base(pin);

	// Calculate register number under GPIO Group
	uint8_t pin_number = pin % REGISTERS_PER_GROUP;

	// Get registers values and use clear offset
	volatile uint32_t *register_address = reg32(base, GPIO_CLEARDATAOUT_OFF);
	// Set register value to 1 to clear
	*register_address = (1U << pin_number);
}

/*--------------------------------------
 * Function: gpio_set_direction
 *--------------------------------------*/
void gpio_set_direction(uint8_t pin, const char *direction) {
	// Set gpio direction based on direction char
	if (strcmp(direction, GPIO_IN) == 0) {
		// Set GPIO to be input
		gpio_set_direction_in(pin);
	} else {
		// Set GPIO to be output
		gpio_set_direction_out(pin);
	}
}

/*--------------------------------------
 * Function: gpio_set_direction_out
 *--------------------------------------*/
void gpio_set_direction_out(uint8_t pin) {
	// Get Base Address - exits if function fails
	volatile uint8_t *base = get_gpio_base(pin);

	// Calculate register number under GPIO Group
	uint8_t pin_number = pin % REGISTERS_PER_GROUP;

	// Get register values
	volatile uint32_t *register_address = reg32(base, GPIO_OE_OFFSET);

	// Set register to value 0 to set as output pin
	*register_address &= ~(1U << pin_number);
}

/*--------------------------------------
 * Function: gpio_set_direction_in
 *--------------------------------------*/
void gpio_set_direction_in(uint8_t pin) {
	// Get Base Address - exits if function fails
	volatile uint8_t *base = get_gpio_base(pin);

	// Calculate register number under GPIO Group
	uint8_t pin_number = pin % REGISTERS_PER_GROUP;

	// Get register values
	volatile uint32_t *register_address = reg32(base, GPIO_OE_OFFSET);

	// Set register to value 1 to set as output pin
	*register_address |= (1U << pin_number);
}
