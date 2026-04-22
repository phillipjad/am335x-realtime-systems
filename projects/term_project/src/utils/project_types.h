#ifndef PROJECT_TYPES_H
#define PROJECT_TYPES_H

#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#define STATUS_SUCCESS (0)
#define STATUS_FAIL (-1)

typedef double float64_t;

/** State Machine States */
typedef enum { FAIL, AUTO } state_t;

typedef struct {
	uint8_t servo_chip; /**< chip number for servo */
	char servo_channel; /**< channel number for servo */
} servo_t;

typedef struct {
	uint8_t target_temp_led;       /**< Pin for target_temp_led */
	uint8_t system_ok_led;         /**< Pin for system_ok_led */
	uint8_t system_fail_led;       /**< Pin for system_fail_led */
	uint8_t lcd; 		       /**< Pin for LCD screen */
	uint8_t potentiometer; 	       /**< Pin for potentiometer */
	servo_t servo;       	       /**< Pin for servo */
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

/**
 * @brief Global struct used to share values across various threads
 */
typedef struct {
	pthread_mutex_t mutex;             /**< Mutex for protected access */
	pthread_cond_t cv;                 /**< Condition variable to control threads */
	atomic_bool is_shutdown_requested; /**< atomic_bool to determine if we should shutdown the application */
	configuration_items_t config;      /**< Configuration items used throughout application */
	state_t current_state;             /**< System state */
	float64_t current_temp;            /**< Current temperature */
	float64_t target_temp;             /**< Target temperature */
	bool servo_health;                 /**< Servo health */
	bool temperature_health;           /**< Temperature health */
	bool relay_health;           	   /**< Relay health */
	uint64_t heartbeats[NUM_THREADS];
#ifndef NDEBUG
#endif                              /* NDEBUG */
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
