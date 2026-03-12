#include "sensor_monitoring.h"

#include <pthread.h>
#include <time.h>

/* Local project includes after system libraries */
#include "../utils/app_config.h"
#include "../utils/gpio_control.h"
#include "../utils/logger.h"
#include "../utils/project_constants.h"
#include "../utils/project_types.h"

extern configuration_items_t user_config;

/*--------------------------------------
 * Function: read_with_debounce
 *--------------------------------------*/
static bool read_with_debounce(button_debounce_t *d) {
	uint8_t raw = gpio_read(d->pin_id);
	bool pressed = false;

	// Check for button press
	if (raw == d->last_raw) {
		d->stable_count++;
	} else {
		d->stable_count = 0;
	}
	d->last_raw = raw;

	// Check if we reached the stable count
	if (d->stable_count == STABLE_NEEDED) {
		d->debounce = raw;
		// Check for falling edge: 1 -> 0 means "pressed"
		if (d->prev_debounced == 1 && d->debounce == 0) {
			pressed = true;
		}
		d->prev_debounced = d->debounce;
	}
	return pressed;
}

/*------------------------
 * Function: time_taken
 *------------------------*/
static float64_t time_taken(struct timespec *start, struct timespec *end) {
	float64_t seconds = (float64_t)end->tv_sec - start->tv_sec;
	float64_t nseconds = (float64_t)(end->tv_nsec - start->tv_nsec) / SEC_TO_NSEC;
	return seconds + nseconds;
}

/*-----------------------------
 * Function: sensor_monitoring
 *-----------------------------*/
void sensor_monitoring(global_values_t *shared_info, direction_t train_direction) {
	// Grab snapshot of state and direction at start
	// Grab lock
	pthread_mutex_lock(&shared_info->mutex);
	state_t snapshot_state = shared_info->current_state;
	direction_t snapshot_direction = shared_info->current_direction;
	struct timespec snapshot_arrival_time = shared_info->arrival_time;
	// Release lock
	pthread_mutex_unlock(&shared_info->mutex);

	// Check state
	// IDLE means train is coming from the button direction
	if (snapshot_state == STATE_IDLE) {
		// Grab lock
		pthread_mutex_lock(&shared_info->mutex);
		// If state is still idle even after potential context switch
		if (shared_info->current_state == STATE_IDLE) {
			// Update State to active - Train is arriving
			shared_info->current_state = STATE_ACTIVE;
			// Store direction_t
			shared_info->current_direction = train_direction;
			// 5 second timer - log current time
			clock_gettime(CLOCK_MONOTONIC_RAW, &shared_info->arrival_time);
			// Wake up lights and servo threads
			pthread_cond_broadcast(&shared_info->cv);
		}
		// Release lock
		pthread_mutex_unlock(&shared_info->mutex);
		// Log train direction
		LOG("Train arriving from the %s", (train_direction == DIRECTION_EAST ? "EAST" : "WEST"));

		// Check if the train has arrived to the other side
		// ACTIVE means train is already on track
	} else if (snapshot_state == STATE_ACTIVE) {
		// Struct containing time of second button press (either same or other side)
		struct timespec second_button_time = { 0 };
		// End 5 second timer - log current time
		clock_gettime(CLOCK_MONOTONIC_RAW, &second_button_time);
		// Calculate time taken
		float64_t overall_time = time_taken(&snapshot_arrival_time, &second_button_time);
		// Used for log flow
		bool fail_safe_active = false;

		// Grab lock
		pthread_mutex_lock(&shared_info->mutex);
		// If state is still active even after potential context switch
		if (shared_info->current_state == STATE_ACTIVE) {
			// Check if train direction is still on the same button or has arrived to the next button
			if (snapshot_direction == train_direction || overall_time > (float64_t)TIMEOUT_TIME) {
				// Train has not moved since last button press
				shared_info->current_state = STATE_FAIL_SAFE;
				fail_safe_active = true;

				// Train has moved to other side of the platform within 5 seconds
			} else {
				// Set state to clear and clear global variables
				shared_info->current_state = STATE_CLEARING;
				shared_info->current_direction = DIRECTION_NONE;
				// Not clearing end time for servo activation after 1 second
				shared_info->arrival_time = (struct timespec){ 0 };
				// Set clear time
				shared_info->clear_time = second_button_time;
			}
			// Send signal to sleeping threads (LEDs and servo)
			pthread_cond_broadcast(&shared_info->cv);
		}
		// Release lock
		pthread_mutex_unlock(&shared_info->mutex);
		// Logs outside critical section
		if (fail_safe_active) {
			LOG("FAILSAFE STATE ACTIVE: Train has not moved from the %s. Lowering gate and warning lights blinking. "
			    "Awaiting supervisor clear...",
			(train_direction == DIRECTION_EAST ? "EAST" : "WEST"));
		} else {
			LOG("CLEAR STATE ACTIVE: Train has arrive to other end of platform. Opening gate and turning off lights.");
		}
	}
}

/*-----------------------------
 * Function: sensor_monitoring
 *-----------------------------*/
void failsafe_timout(global_values_t *shared_info) {
	// Grab snapshot of state and direction at start
	// Grab lock
	pthread_mutex_lock(&shared_info->mutex);
	state_t snapshot_state = shared_info->current_state;
	direction_t snapshot_direction = shared_info->current_direction;
	struct timespec snapshot_arrival_time = shared_info->arrival_time;
	// Release lock
	pthread_mutex_unlock(&shared_info->mutex);

	// Check if in active state and longer than 5 seconds
	if (snapshot_state == STATE_ACTIVE) {
		struct timespec current_time = { 0 };
		// Grab current amount of time passed
		clock_gettime(CLOCK_MONOTONIC_RAW, &current_time);
		float64_t overall_time = time_taken(&snapshot_arrival_time, &current_time);

		// If longer than 5 seconds go to fail-safe state
		if (overall_time > (float64_t)TIMEOUT_TIME) {
			// Used for log flow
			bool fail_safe_active = false;
			// Grab lock
			pthread_mutex_lock(&shared_info->mutex);

			// If state is still active even after potential context switch
			if (shared_info->current_state == STATE_ACTIVE) {
				// Enter fail-safe state
				shared_info->current_state = STATE_FAIL_SAFE;
				// Signal to sleeping threads (LEDS and servo)
				pthread_cond_broadcast(&shared_info->cv);
			}
			// Release lock
			pthread_mutex_unlock(&shared_info->mutex);
			if (fail_safe_active) {
				LOG("FAILSAFE TIMEOUT ACTIVE: Train did not arrive within %d seconds.", TIMEOUT_TIME);
			}
		}
	}
}

/*------------------------------------------
 * Function: sensor_monitoring_thread_entry
 *------------------------------------------*/
void *sensor_monitoring_thread_entry(void *arg) {
	LOG("Starting sensor_monitoring!");
	global_values_t *shared_info = (global_values_t *)arg;
	struct timespec timer = { 0 };
	// Waiting period using debounce parameter
	timer.tv_sec = SAMPLE_MS / MSEC_PER_SEC;
	timer.tv_nsec = (SAMPLE_MS % MSEC_PER_SEC) * NSEC_PER_MSEC;

	// Button setup
	button_debounce_t east_button = { user_config.gpio_layout.east_button, 0, 0, 0, 0 };
	button_debounce_t west_button = { user_config.gpio_layout.west_button, 0, 0, 0, 0 };

	// Check for button press
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		// Check west approach button
		if (read_with_debounce(&west_button)) {
			sensor_monitoring(shared_info, DIRECTION_WEST);
		}

		if (read_with_debounce(&east_button)) {
			sensor_monitoring(shared_info, DIRECTION_EAST);
		}

		// Failsafe initiated
		failsafe_timout(shared_info);
		// sleep
		nanosleep(&timer, NULL);
	}
	LOG("Shutting down sensor monitoring thread...");
	return NULL;
}
