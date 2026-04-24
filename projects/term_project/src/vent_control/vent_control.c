#include "vent_control.h"

#include <pthread.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "heartbeat.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "servo_controller.h"

static global_values_t *shared_info = NULL;
static bool vent_open = false;

/*------------------------
 * Function: time_taken
 *------------------------*/
static float64_t time_taken(struct timespec *start, struct timespec *end) {
	time_t seconds = end->tv_sec - start->tv_sec;
	int64_t nanoseconds = (end->tv_nsec - start->tv_nsec) / (int64_t)SEC_TO_NSEC;
	float64_t seconds_as_float = (float64_t)seconds;
	float64_t nseconds_as_float = (float64_t)nanoseconds;
	return seconds_as_float + nseconds_as_float;
}

static void handle_vent_logic(void) {
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		sleep(2U);
	}

	if (atomic_load(&shared_info->is_shutdown_requested)) {
		return;
	}

	bool state_updated = false;
	/* Grab state snapshot */
	pthread_mutex_lock(&shared_info->mutex);
	state_t current_state = shared_info->current_state;
	float64_t current_temp = shared_info->current_temp;
	float64_t target_temp = shared_info->target_temp;
	pthread_mutex_unlock(&shared_info->mutex);
	struct timespec servo_start_time = { 0 };
	struct timespec servo_end_time = { 0 };

	// Check if shutdown requested or state changed from idle
	if (!atomic_load(&shared_info->is_shutdown_requested)) {
		// If operating automatically
		if (current_state == STATE_RUNNING) {
			// Check if temp changed and vent needs to be opened/closed
			if ((current_temp > (target_temp + TEMP_BUFFER)) && !vent_open) {
				LOG("vent read STATE_RUNNING state with high temp, opening vent.");
				(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_start_time);
				servo_raise();
				(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_end_time);
				vent_open = true;
				state_updated = true;
			} else if ((current_temp < (target_temp - TEMP_BUFFER)) && vent_open) {
				LOG("vent read STATE_RUNNING state with low temp, closing vent.");
				(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_start_time);
				servo_lower();
				(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_end_time);
				vent_open = false;
				state_updated = true;
			} else {
				/* MISRA requires else */
			}
		} else if (current_state == STATE_FAIL_SAFE) {
			// If idle map potentiometer reading to servo angle: SERVO_LOWER + (potentiometer_activation_percentage * (SERVO_RAISE - SERVO_LOWER))
			if (!vent_open) {
				LOG("vent read STATE_RUNNING state with manual control, closing vent.");
				(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_start_time);
				servo_lower();
				(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_end_time);
			}
		} else if (current_state == STATE_FAIL) {
			// Send message to queue and wait for it to finish
			LOG_AND_EXIT("vent read STATE_FAIL, shutting down");
		} else {
			/* MISRA requires else */
		}
		if ((time_taken(&servo_start_time, &servo_end_time) > SERVO_TIMEOUT_MS_TIME_F) && state_updated)  {
			pthread_mutex_lock(&shared_info->mutex);
			shared_info->current_state = STATE_FAIL;
			shared_info->servo_health = false;
			pthread_mutex_unlock(&shared_info->mutex);
		}
	} else {
		return;
	}
}

/*--------------------------------------
 * Function: vent_control_thread_entry
 *--------------------------------------*/
void *vent_control_thread_entry(void *arg) {
	LOG("Starting vent control thread");
	shared_info = (global_values_t *)arg;
	// Assign current starting state
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		handle_vent_logic();
		increment_heartbeat(shared_info, VENT_CONTROL);
	}
	LOG("Shutting down vent control thread");
	return NULL;
}
