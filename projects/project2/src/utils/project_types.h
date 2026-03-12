#ifndef PROJECT_TYPES_H
#define PROJECT_TYPES_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "pthread.h"

#define STATUS_SUCCESS (0)
#define STATUS_FAIL (-1)

typedef double float64_t;

/* Train Directions */
typedef enum { DIRECTION_EAST, DIRECTION_WEST, DIRECTION_NONE, DIRECTION_UNKOWN } direction_t;

/* State Machine States */
typedef enum { STATE_IDLE, STATE_ACTIVE, STATE_CLEARING, STATE_FAIL_SAFE } state_t;

typedef struct {
	uint8_t east_button; /**< Pin for east button */
	uint8_t west_button; /**< Pin for west button */
	uint8_t led_1;       /**< Pin for led_1 */
	uint8_t led_2;       /**< Pin for led_2 */
	uint8_t servo;       /**< Pin for servo */
} gpio_layout_t;

typedef struct {
	gpio_layout_t gpio_layout;
} configuration_items_t;

// Global variables
typedef struct {
	// lock critical sections
	pthread_mutex_t mutex;
	// condition variable to control threads
	pthread_cond_t cv;
	atomic_bool is_shutdown_requested;

	// system state
	state_t current_state;
	// train direction
	direction_t current_direction;
	// train arrival time
	struct timespec arrival_time;
	// train clear time
	struct timespec clear_time;
} global_values_t;

// Button debounce variables
typedef struct {
	uint8_t pin_id;
	int last_raw;
	int stable_count;
	int debounce;
	int prev_debounced;
} button_debounce_t;

typedef struct {
	int fd;
	volatile uint8_t *map_base;  // mapped page base
	volatile uint8_t *gpio_base; // base + page_offset
} gpio_map_t;

#endif /* PROJECT_TYPES_H */
