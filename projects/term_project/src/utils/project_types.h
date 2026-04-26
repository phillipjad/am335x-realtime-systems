#ifndef PROJECT_TYPES_H
#define PROJECT_TYPES_H

#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "project_constants.h"

#define STATUS_SUCCESS (0)
#define STATUS_FAIL (-1)

typedef double float64_t;

/** State Machine States */
typedef enum { STATE_FAIL, STATE_FAIL_SAFE, STATE_RUNNING, STATE_IDLE } state_e;

typedef struct {
	uint8_t servo_chip; /**< chip number for servo */
	char servo_channel; /**< channel number for servo */
} servo_t;

typedef struct {
	uint8_t target_temp_led; /**< Pin for target_temp_led */
	uint8_t system_ok_led;   /**< Pin for system_ok_led */
	uint8_t system_fail_led; /**< Pin for system_fail_led */
	uint8_t lcd_i2c_bus;     /**< Bus for LCD screen */
	int32_t lcd_fd;          /**< file descriptor for LCD screen */
	uint8_t potentiometer;   /**< Pin for potentiometer */
	servo_t servo;           /**< Pin for servo */
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
	int32_t fd;
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

typedef struct {
	bool is_set;
	char error_msg[MAX_LOG_LEN + 1U];
} error_e;

/**
 * @brief Global struct used to share values across various threads
 */
typedef struct {
	pthread_mutex_t mutex;                     /**< Mutex for protected access */
	pthread_cond_t cv;                         /**< Condition variable to control threads */
	atomic_bool is_shutdown_requested;         /**< atomic_bool to determine if we should shutdown the application */
	configuration_items_t config;              /**< Configuration items used throughout application */
	state_e current_state;                     /**< System state */
	float64_t current_temp;                    /**< Current temperature */
	float64_t target_temp;                     /**< Target temperature */
	float64_t potentiometer_percentage_closed; /**< Potentiometer value used for manual control */
	struct timespec servo_activation_time;     /**< Train arrival time */
	uint64_t heartbeats[NUM_THREADS];          /**< Thread heartbeats */
	error_e thread_errors[NUM_THREADS];        /**< Thread errors */
	log_queue_t logger;                        /**< Application logger */
} global_values_t;

static inline bool has_error(const error_e *err) {
	return err->is_set;
}

static inline void set_error(error_e *err, const char *error_msg) {
	err->is_set = true;
	(void)strncpy(err->error_msg, error_msg, MAX_LOG_LEN);
}

static inline void clear_error(error_e *err) {
	err->is_set = false;
	(void)memset(err->error_msg, 0, MAX_LOG_LEN + 1U);
}

#endif /* PROJECT_TYPES_H */
