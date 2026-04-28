#include "fan_control.h"
#include <time.h>

/* Local project includes after system libraries */
#include "fan_controller.h"
#include "gpio_control.h"
#include "heartbeat.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "thread_utils.h"

static global_values_t *shared_info = NULL;

#define FAN_DELTA_MAX (10.0) /* delta at/above which fan runs at 100% */
#define FAN_DELTA_OFF (-5.0) /* delta at/below which fan turns off (hysteresis) */

/* 4-pin PWM fan specification: 2 tach pulses per revolution
 * based on https://www.nidec.com/en/technology/capability/thermal/article/0026/ */
#define TACH_PULSES_PER_REV (2.0)

typedef struct {
	float64_t current_temp;
	float64_t target_temp;
	state_e current_state;
} fan_inputs_t;

static fan_inputs_t read_fan_inputs(void) {
	fan_inputs_t in = { 0 };
	pthread_mutex_lock(&shared_info->mutex);
	in.current_temp = shared_info->current_temp;
	in.target_temp = shared_info->target_temp;
	in.current_state = shared_info->current_state;
	pthread_mutex_unlock(&shared_info->mutex);
	return in;
}

static uint8_t compute_fan_speed_pct(fan_inputs_t in) {
	static bool fan_is_active = false;

	if ((in.current_state == STATE_IDLE) || (in.current_state == STATE_FAIL)) {
		fan_is_active = false;
		return (uint8_t)0;
	}

	float64_t delta = in.current_temp - in.target_temp;

	if (delta > 0.0) {
		fan_is_active = true;
	} else if (delta <= FAN_DELTA_OFF) {
		fan_is_active = false;
	} else {
		/* -5.0 < delta <= 0.0: hysteresis zone, maintain fan_is_active */
	}

	if (!fan_is_active) {
		return (uint8_t)0;
	}
	if (delta >= FAN_DELTA_MAX) {
		return (uint8_t)100;
	}
	if (delta > 0.0) {
		return (uint8_t)(delta / FAN_DELTA_MAX * 100.0);
	}
	return (uint8_t)0;
}

static void apply_fan_speed(uint8_t speed_pct) {
	int32_t result = fan_set_speed_pct(speed_pct);
	pthread_mutex_lock(&shared_info->mutex);
	if (result != STATUS_SUCCESS) {
		set_error(&shared_info->thread_errors[FAN_CONTROL], "fan_set_speed_pct failed for %u%%", speed_pct);
	} else {
		clear_error(&shared_info->thread_errors[FAN_CONTROL]);
	}
	pthread_mutex_unlock(&shared_info->mutex);
}

static void measure_and_log_rpm(uint8_t tach_pin) {
	int32_t saved_priority = 0;
	/* Attempt to minimize bit bang misses by reducing preemption likelihood */
	boost_thread_priority(&saved_priority);

	struct timespec start = { 0 };
	clock_gettime(CLOCK_MONOTONIC, &start);

	struct timespec window_end = start;
	window_end.tv_nsec += QUARTER_SECOND_AS_NSEC;
	if (window_end.tv_nsec >= (long)NSEC_PER_SEC) {
		window_end.tv_sec += 1L;
		window_end.tv_nsec -= (long)NSEC_PER_SEC;
	}

	uint32_t rising_edges = 0U;
	uint8_t prev_val = gpio_read(tach_pin);
	struct timespec now = { 0 };
	do {
		uint8_t curr_val = gpio_read(tach_pin);
		if ((prev_val == 0U) && (curr_val != 0U)) {
			++rising_edges;
		}
		prev_val = curr_val;
		clock_gettime(CLOCK_MONOTONIC, &now);
	} while ((now.tv_sec < window_end.tv_sec) || ((now.tv_sec == window_end.tv_sec) && (now.tv_nsec < window_end.tv_nsec)));

	restore_thread_priority(saved_priority);

	float64_t elapsed_secs = (float64_t)(now.tv_sec - start.tv_sec) + ((float64_t)(now.tv_nsec - start.tv_nsec) / NSEC_PER_SEC_F);
	float64_t rpm = ((float64_t)rising_edges / TACH_PULSES_PER_REV) * (60.0 / elapsed_secs);
	LOG(FAN_CONTROL, "Fan RPM: %.0f", rpm);
}

void *fan_control_thread_entry(void *arg) {
	LOG(FAN_CONTROL, "Starting fan control thread");
	shared_info = (global_values_t *)arg;
	uint8_t tach_pin = shared_info->config.pin_layout.fan_tach;

	static const struct timespec thread_sleep = { .tv_sec = 0L, .tv_nsec = 500000000L };
	uint8_t iteration = 0U;

	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		fan_inputs_t inputs = read_fan_inputs();
		uint8_t speed_pct = compute_fan_speed_pct(inputs);
		apply_fan_speed(speed_pct);

		if ((++iteration % 10U) == 1U) {
			measure_and_log_rpm(tach_pin);
		}

		increment_heartbeat(shared_info, FAN_CONTROL);
		nanosleep(&thread_sleep, NULL);
	}

	fan_set_speed_pct(0U);
	LOG(FAN_CONTROL, "Shutting down fan control thread");
	return NULL;
}
