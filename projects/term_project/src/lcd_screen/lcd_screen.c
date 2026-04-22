#include "lcd_screen.h"
#include <unistd.h>

/* Local project includes after system libraries */
#include "heartbeat.h"
#include "logger.h"
#include "project_types.h"

/** A quarter of a second represented in nanoseconds */
#define QUARTER_SECOND_AS_NSEC (250000000L)

static float64_t get_current_time() {
        struct timespec t = {0};
        (void)clock_gettime(CLOCK_MONOTONIC_RAW, &t);
        return t.tv_sec + ((t.tv_nsec) / (int64_t)SEC_TO_NSEC);
}

static global_values_t *shared_info = NULL;

void *lcd_screen_thread_entry(void *arg) {
	LOG("Starting LCD screen thread");
	struct timespec timer = { 0 };
	timer.tv_nsec = QUARTER_SECOND_AS_NSEC;
	shared_info = (global_values_t *)arg;
	float64_t latest_target_temp = 0;
	float64_t latest_target_temp_timestamp = 0;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		pthread_mutex_lock(&shared_info->current_state);
		float64_t current_temp = shared_info->current_temp;
		float64_t target_temp = shared_info->target_temp;
		increment_heartbeat(shared_info, LCD_SCREEN);

		// Check if target temp changed
		if (latest_target_temp != target_temp) {
			latest_target_temp = target_temp;
			latest_target_temp_timestamp = get_current_time();
		}

		if (shared_info->current_state == STATE_AUTO) {
			float64_t current_time = get_current_time();

			if ((current_time - latest_target_timestamp) <= 2.00) {
				// Print on LCD - Target Temp: ##
			} else {
				// Print on LCD - Actual temperature: ###
			}
		} else if (shared_info->current_state == STATE_FAIL_SAFE) {
			// Print on LCD - ERROR: LCD SCREEN STATE FAIL-SAFE
		} else {

			// Print on LCD - ERROR: LCD SCREEN STATE FAILURE
		}
		nanosleep(&timer, NULL);
	}
	LOG("Shutting down LCD screen thread");
	return NULL;

}
