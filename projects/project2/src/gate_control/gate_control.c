#include "gate_control.h"

#include <pthread.h>
#include <time.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "servo_controller.h"

static global_values_t *shared_info = NULL;

static void handle_gate_logic(void) {
	/* We want to track what the last state we acted on is */
	static state_t last_operated_state = STATE_INVALID;
	/* We want to track the last time the warning lights were deactivated */
	static struct timespec last_lights_off_time = { 0 };
	/* Grab state snapshot */
	pthread_mutex_lock(&shared_info->mutex);
	state_t current_state = shared_info->current_state;
	// While state is same as initializing state, wait on cv
	while ((current_state == last_operated_state) && (!atomic_load(&shared_info->is_shutdown_requested))) {
		pthread_cond_wait(&shared_info->cv, &shared_info->mutex);
		/* Grab updated state snapshot while we have the mutex */
		current_state = shared_info->current_state;
	}
	pthread_mutex_unlock(&shared_info->mutex);

	// Check if shutdown requested or state changed from idle
	if (!atomic_load(&shared_info->is_shutdown_requested)) {
		if (current_state == STATE_IDLE) {
			servo_raise();
			LOG("Gate read IDLE state, raising gate.");
		} else if (current_state == STATE_ACTIVE) {
			servo_lower();
			LOG("Gate read STATE_ACTIVE state, lowering gate.");
		} else if (current_state == STATE_FAIL_SAFE) {
			servo_lower();
			LOG("Gate read FAIL_STATE state, lowering gate.");
		} else if (current_state == STATE_CLEARING) {
			LOG("Gate read CLEARING_STATE state");
			/* Get current time */
			struct timespec curr_time = { 0 };
			(void)clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
			struct timespec lights_off_time = { 0 };
			/* Wait for warning light module to update the light off timestamp */
			do {
				pthread_mutex_lock(&shared_info->mutex);
				lights_off_time = shared_info->lights_off_time;
				pthread_mutex_unlock(&shared_info->mutex);

				/* Sleep for 25 milliseconds between each query */
				static const struct timespec delay_timer = { .tv_sec = 0, .tv_nsec = (25L * NSEC_PER_MSEC) };
				(void)nanosleep(&delay_timer, NULL);
			} while ((last_lights_off_time.tv_sec == lights_off_time.tv_sec) &&
			    (last_lights_off_time.tv_nsec == lights_off_time.tv_nsec));
			last_lights_off_time = lights_off_time;
			/* Once warning lights are off we want to wait for one second to pass and then raise gate */
			static const struct timespec one_second_delay = { .tv_sec = 1, .tv_nsec = 0 };
			(void)nanosleep(&one_second_delay, NULL);
			servo_raise();
			LOG("Waited and raising gate.");
			return;
		}
		last_operated_state = current_state;
	} else {
		return;
	}
}

/*--------------------------------------
 * Function: gate_control_thread_entry
 *--------------------------------------*/
void *gate_control_thread_entry(void *arg) {
	LOG("Starting gate control thread");
	shared_info = (global_values_t *)arg;
	// Assign current starting state
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		handle_gate_logic();
	}
	LOG("Shutting down gate control thread");
	return NULL;
}
