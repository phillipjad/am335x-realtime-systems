#ifndef PROJECT_TYPES_H
#define PROJECT_TYPES_H

#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "project_constants.h"

#define STATUS_SUCCESS (0)
#define STATUS_FAIL (-1)

typedef double float64_t;

/** Train Directions */
typedef enum { DIRECTION_EAST, DIRECTION_WEST, DIRECTION_NONE, DIRECTION_UNKOWN } direction_t;

/** State Machine States */
typedef enum { STATE_IDLE, STATE_ACTIVE, STATE_CLEARING, STATE_FAIL_SAFE, STATE_INVALID } state_t;

typedef struct {
	uint8_t servo_chip; /**< chip number for servo */
	char servo_channel; /**< channel number for servo */
} servo_t;

typedef struct {
	uint8_t east_button; /**< Pin for east button */
	uint8_t west_button; /**< Pin for west button */
	uint8_t led_1;       /**< Pin for led_1 */
	uint8_t led_2;       /**< Pin for led_2 */
	servo_t servo;       /**< Pin for servo */
} gpio_layout_t;

typedef struct {
	gpio_layout_t gpio_layout;
} configuration_items_t;

typedef enum {
	VENT_CONTROL,
	LOG_HANDLER,
	SENSOR_MONITORING,
	SUPERVISOR_INPUT,
	LCD_SCREEN,
	TEMP_SENSOR,
	LED,
	POTENTIOMETER,
	STATE_MANAGEMENT,
	NUM_THREADS
} thread_index_e;

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

typedef struct {
	thread_index_e thread_id;
	bool include_newline;
	char filename[MAX_FILENAME_LENGTH + 1U];
	uint32_t line_no;
	char message[MAX_LOG_LEN + 1U];
} log_queue_message_t;

typedef struct {
	pthread_mutex_t log_mu;
	pthread_cond_t log_cv;
	log_queue_message_t queue[MAX_LOG_QUEUE_CAPACITY];
	size_t head;
	size_t tail;
	size_t size;
} log_queue_t;

/**
 * @brief Global struct used to share values across various threads
 */
typedef struct {
	pthread_mutex_t mutex;             /**< Mutex for protected access */
	pthread_cond_t cv;                 /**< Condition variable to control threads */
	atomic_bool is_shutdown_requested; /**< atomic_bool to determine if we should shutdown the application */
	configuration_items_t config;      /**< Configuration items used throughout application */
	state_t current_state;             /**< System state */
	direction_t current_direction;     /**< Train direction */
	struct timespec arrival_time;      /**< Train arrival time */
	struct timespec clear_time;        /**< Train clear time */
	struct timespec lights_off_time;   /**< Last time that lights were turned off */
	uint64_t heartbeats[NUM_THREADS];  /**< Tracks thread heartbeats */
	log_queue_t logger;                /**< Application logger */
#ifndef NDEBUG
	atomic_bool debug_east_pending; /**< Debug mode: pending simulated east button press */
	atomic_bool debug_west_pending; /**< Debug mode: pending simulated west button press */
#endif                              /* NDEBUG */
} global_values_t;

#endif /* PROJECT_TYPES_H */
