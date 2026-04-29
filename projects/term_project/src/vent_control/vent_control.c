#include "vent_control.h"

#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "heartbeat.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "servo_controller.h"

static global_values_t *shared_info = NULL;
static float64_t last_vent_pct = -1.0;

#define VENT_DELTA_MAX (10.0)   /* delta at/above which vent is 100% open */
#define VENT_DELTA_CLOSE (-5.0) /* delta at/below which vent closes (hysteresis) */

/*---------------------------
 * Function: time_taken in ms
 *---------------------------*/
static int64_t time_taken(struct timespec *start, struct timespec *end) {
	time_t seconds = (end->tv_sec - start->tv_sec);
	int64_t nanoseconds = end->tv_nsec - start->tv_nsec;
	int64_t milliseconds = (nanoseconds / (int64_t)NSEC_PER_MSEC);
	milliseconds += ((int64_t)seconds * MSEC_PER_SEC);
	return milliseconds;
}

/*---------------------------
 * Function: compute_vent_open_pct
 *---------------------------*/
static float64_t compute_vent_open_pct(state_e current_state, float64_t delta) {
	static bool vent_is_active = false;

	if ((current_state == STATE_IDLE) || (current_state == STATE_FAIL)) {
		vent_is_active = false;
		return 0.0;
	}

	if (delta > 0.0) {
		vent_is_active = true;
	} else if (delta <= VENT_DELTA_CLOSE) {
		vent_is_active = false;
	} else {
		/* -5.0 < delta <= 0.0: hysteresis zone, maintain vent_is_active */
	}

	if (!vent_is_active) {
		return 0.0;
	}
	if (delta >= VENT_DELTA_MAX) {
		return 100.0;
	}
	if (delta > 0.0) {
		return (delta / VENT_DELTA_MAX) * 100.0;
	}
	return 0.0;
}

/*---------------------------
 * Function: apply_vent_position
 *---------------------------*/
static void apply_vent_position(float64_t open_pct) {
	struct timespec servo_start = { 0 };
	struct timespec servo_end = { 0 };
	(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_start);
	int32_t result = potentiometer_based_servo(open_pct);
	(void)clock_gettime(CLOCK_MONOTONIC_RAW, &servo_end);

	pthread_mutex_lock(&shared_info->mutex);
	if (time_taken(&servo_start, &servo_end) > SERVO_TIMEOUT_MS_TIME_F) {
		set_error(&shared_info->thread_errors[VENT_CONTROL], "Servo is unresponsive and system has failed");
	} else if (result != STATUS_SUCCESS) {
		set_error(&shared_info->thread_errors[VENT_CONTROL], "Servo failed to move");
	} else {
		clear_error(&shared_info->thread_errors[VENT_CONTROL]);
	}
	pthread_mutex_unlock(&shared_info->mutex);
}

static void handle_vent_logic(void) {
	if (atomic_load(&shared_info->is_shutdown_requested)) {
		return;
	}

	pthread_mutex_lock(&shared_info->mutex);
	increment_heartbeat(shared_info, VENT_CONTROL);
	state_e current_state = shared_info->current_state;
	float64_t current_temp = shared_info->current_temp;
	float64_t target_temp = shared_info->target_temp;
	float64_t potentiometer_pct = shared_info->potentiometer_percentage_closed;
	pthread_mutex_unlock(&shared_info->mutex);

	if (atomic_load(&shared_info->is_shutdown_requested)) {
		return;
	}

	float64_t vent_pct;
	if (current_state == STATE_RUNNING) {
		float64_t delta = current_temp - target_temp;
		vent_pct = compute_vent_open_pct(current_state, delta);
		if (vent_pct != last_vent_pct) {
			LOG(VENT_CONTROL, "Adjusting vent to %.1f%% open (delta: %.2f F)", vent_pct, delta);
		}
	} else if (current_state == STATE_FAIL_SAFE) {
		vent_pct = potentiometer_pct;
		if (vent_pct != last_vent_pct) {
			LOG(VENT_CONTROL, "Read STATE_FAIL_SAFE state with manual control: %.2f%%", vent_pct);
		}
	} else {
		vent_pct = compute_vent_open_pct(current_state, 0.0);
	}

	last_vent_pct = vent_pct;
	apply_vent_position(vent_pct);
}

/*--------------------------------------
 * Function: vent_control_thread_entry
 *--------------------------------------*/
void *vent_control_thread_entry(void *arg) {
	LOG(VENT_CONTROL, "Starting vent control thread");
	shared_info = (global_values_t *)arg;
	struct timespec thread_sleep = { .tv_sec = 0L, .tv_nsec = 500 * NSEC_PER_MSEC };
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		handle_vent_logic();
		(void)nanosleep(&thread_sleep, NULL);
	}
	LOG(VENT_CONTROL, "Shutting down vent control thread");
	return NULL;
}
