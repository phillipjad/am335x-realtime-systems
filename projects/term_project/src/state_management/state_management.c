#include "state_management.h"
#include <pthread.h>
#include <string.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "heartbeat.h"
#include "logger.h"
#include "project_types.h"

static global_values_t *shared_info = NULL;

static bool is_terminal_thread_error(const char *thread_name) {
	/* If additional thread errors are terminal for the system they should be added here */
	return (strncmp(thread_name, "vent_controller", 50U) == 0);
}

static void handle_application_health(void) {
	/* All errors are able to operate in fail-safe except for servo */
	error_e error_copy[NUM_THREADS] = { 0 };
	pthread_mutex_lock(&shared_info->mutex);
	(void)memcpy(error_copy, shared_info->thread_errors, NUM_THREADS * sizeof(error_e));
	pthread_mutex_unlock(&shared_info->mutex);

	bool error_present = false;
	for (uint8_t ii = 0U; ii < NUM_THREADS; ++ii) {
		if (error_copy[ii].is_set) {
			error_present = true;
			LOG((thread_index_e)ii, "%s", error_copy[ii].error_msg);
			if (is_terminal_thread_error(THREAD_NAMES[ii])) {
				pthread_mutex_lock(&shared_info->mutex);
				shared_info->current_state = STATE_FAIL;
				pthread_mutex_unlock(&shared_info->mutex);
				struct timespec lcd_update = { .tv_sec = 1L, .tv_nsec = 0L };
				nanosleep(&lcd_update, NULL);
				atomic_store(&shared_info->is_shutdown_requested, true);
				break;
			}

			pthread_mutex_lock(&shared_info->mutex);
			shared_info->current_state = STATE_FAIL_SAFE;
			pthread_mutex_unlock(&shared_info->mutex);
		}
	}

	if (!error_present) {
		pthread_mutex_lock(&shared_info->mutex);
		shared_info->current_state = STATE_RUNNING;
		pthread_mutex_unlock(&shared_info->mutex);
	}
}

void *state_management_thread_entry(void *arg) {
	LOG(STATE_MANAGEMENT, "Starting state management thread");
	shared_info = (global_values_t *)arg;
	static const struct timespec thread_sleep = { .tv_sec = 0, .tv_nsec = 100 * NSEC_PER_MSEC };
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		handle_application_health();
		increment_heartbeat(shared_info, STATE_MANAGEMENT);
		(void)nanosleep(&thread_sleep, NULL);
	}
	LOG(STATE_MANAGEMENT, "Shutting down state management thread");
	return NULL;
}
