#include "vent_control.h"

#include <pthread.h>
#include <string.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "heartbeat.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "servo_controller.h"

static global_values_t *shared_info = NULL;
static bool vent_open = false;
static float64_t last_vent_percent_closed = -1;

/*---------------------------
 * Function: time_taken in ms
 *---------------------------*/
static int64_t time_taken(struct timespec *start, struct timespec *end) {
	time_t seconds = (end->tv_sec - start->tv_sec);
	int64_t nanoseconds = (end->tv_nsec - start->tv_nsec) / (int64_t)NSEC_PER_MSEC;
	int64_t milliseconds = (nanoseconds / NSEC_PER_MSEC);
	milliseconds += ((int64_t)seconds * MSEC_PER_SEC);
	return milliseconds;
}

static void handle_vent_logic(void) {
	if (atomic_load(&shared_info->is_shutdown_requested)) {
		return;
	}

	bool state_updated = false;
	int32_t servo_status = STATUS_SUCCESS;
	/* Grab state snapshot */
	pthread_mutex_lock(&shared_info->mutex);
	increment_heartbeat(shared_info, VENT_CONTROL);
	state_e current_state = shared_info->current_state;
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
				LOG(VENT_CONTROL, "Read STATE_RUNNING state with high temp, opening vent.");
				(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_start_time);
				servo_status = servo_raise();
				(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_end_time);
				vent_open = true;
				state_updated = true;
			} else if ((current_temp < (target_temp - TEMP_BUFFER)) && vent_open) {
				LOG(VENT_CONTROL, "Read STATE_RUNNING state with low temp, closing vent.");
				(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_start_time);
				servo_status = servo_lower();
				(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_end_time);
				vent_open = false;
				state_updated = true;
			} else {
				/* MISRA requires else */
			}
		} else if (current_state == STATE_FAIL_SAFE) {
			// If idle map potentiometer reading to servo angle: SERVO_LOWER + (potentiometer_activation_percentage * (SERVO_RAISE - SERVO_LOWER))
			float64_t read_percent = shared_info->potentiometer_percentage_closed;
			if (last_vent_percent_closed != read_percent) {
				LOG(VENT_CONTROL, "Read STATE_FAIL_SAFE state with manual control: %.2f%% closed", read_percent);
				(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_start_time);
				servo_status = potentiometer_based_servo(read_percent);
				(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_end_time);
				last_vent_percent_closed = read_percent;
				state_updated = true;
			}
		}
		if (state_updated) {
			pthread_mutex_lock(&shared_info->mutex);
			if ((time_taken(&servo_start_time, &servo_end_time) > SERVO_TIMEOUT_MS_TIME_F) && state_updated) {
				set_error(&shared_info->thread_errors[VENT_CONTROL], "Servo is unresponsive and system has failed");
			} else if (servo_status != STATUS_SUCCESS) {
				set_error(&shared_info->thread_errors[VENT_CONTROL], "Servo failed to move");
			} else if (has_error(&shared_info->thread_errors[VENT_CONTROL])) {
				clear_error(&shared_info->thread_errors[VENT_CONTROL]);
			} else {
				/* MISRA requires else */
			}
			pthread_mutex_unlock(&shared_info->mutex);
		}
		/* We don't currently need to clear the servo error since it's terminal but leaving here for future changes */
		// else if ((time_taken(&servo_start_time, &servo_end_time) <= SERVO_TIMEOUT_MS_TIME_F) && state_updated) {
		// 	pthread_mutex_lock(&shared_info->mutex);
		// 	set_error(&shared_info->thread_errors[VENT_CONTROL], "Servo is unresponsive and system has failed");
		// 	pthread_mutex_unlock(&shared_info->mutex);
		// }
	}
}

/*--------------------------------------
 * Function: vent_control_thread_entry
 *--------------------------------------*/
void *vent_control_thread_entry(void *arg) {
	LOG(VENT_CONTROL, "Starting vent control thread");
	shared_info = (global_values_t *)arg;
	struct timespec thread_sleep = { .tv_sec = 0L, .tv_nsec = 500 * NSEC_PER_MSEC };
	// Assign current starting state
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		handle_vent_logic();
		(void)nanosleep(&thread_sleep, NULL);
	}
	LOG(VENT_CONTROL, "Shutting down vent control thread");
	return NULL;
}
