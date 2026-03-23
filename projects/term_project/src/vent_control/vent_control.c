#include "vent_control.h"

#include <pthread.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "servo_controller.h"

static global_values_t *shared_info = NULL;

static void wait_for_warning_light_deactivation(void) {
	/* We want to track the last time the warning lights were deactivated */
	static struct timespec last_lights_off_time = { 0 };
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
		static const struct timespec delay_timer = { .tv_sec = (time_t)0, .tv_nsec = (25L * NSEC_PER_MSEC) };
		(void)nanosleep(&delay_timer, NULL);
	} while ((last_lights_off_time.tv_sec == lights_off_time.tv_sec) && (last_lights_off_time.tv_nsec == lights_off_time.tv_nsec));
	last_lights_off_time = lights_off_time;
}

static void handle_vent_logic(void) {
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		sleep(2U);
	}

	if (atomic_load(&shared_info->is_shutdown_requested)) {
		return;
	}

	/* We want to track what the last state we acted on is */
	static state_t last_operated_state = STATE_INVALID;
	/* Grab state snapshot */
	pthread_mutex_lock(&shared_info->mutex);
	state_t current_state = shared_info->current_state;
	struct timespec arrival_time = shared_info->arrival_time;
	// While state is same as initializing state, wait on cv
	while ((current_state == last_operated_state) && (!atomic_load(&shared_info->is_shutdown_requested))) {
		pthread_cond_wait(&shared_info->cv, &shared_info->mutex);
		/* Grab updated state snapshot while we have the mutex */
		current_state = shared_info->current_state;
		arrival_time = shared_info->arrival_time;
	}
	pthread_mutex_unlock(&shared_info->mutex);

	// Check if shutdown requested or state changed from idle
	if (!atomic_load(&shared_info->is_shutdown_requested)) {
		if (current_state == STATE_IDLE) {
			LOG("vent read IDLE state, raising vent.");
			servo_raise();
		} else if (current_state == STATE_ACTIVE) {
			LOG("vent read STATE_ACTIVE state, lowering vent.");
			struct timespec curr_time = { 0 };
			(void)clock_gettime(CLOCK_MONOTONIC_RAW, &curr_time);
			log_time_difference_ms(curr_time, arrival_time, "lower vent");
			servo_lower();
		} else if (current_state == STATE_FAIL_SAFE) {
			LOG("vent read FAIL_STATE state, lowering vent.");
			/* Once warning lights are off we want to wait for one second to pass and then re-lower vent */
			wait_for_warning_light_deactivation();
			servo_lower();
		} else if (current_state == STATE_CLEARING) {
			LOG("vent read CLEARING_STATE state");
			wait_for_warning_light_deactivation();
			/* Once warning lights are off we want to wait for one second to pass and then raise vent */
			static const struct timespec one_second_delay = { .tv_sec = (time_t)1, .tv_nsec = 0L };
			(void)nanosleep(&one_second_delay, NULL);
			servo_raise();
			LOG("Waited and raising vent.");
			return;
		} else {
			/* MISRA requires else */
		}
		last_operated_state = current_state;
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
	}
	LOG("Shutting down vent control thread");
	return NULL;
}
