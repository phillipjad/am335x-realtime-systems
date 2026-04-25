#include "state_management.h"
#include <pthread.h>
#include <string.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "heartbeat.h"
#include "logger.h"
#include "project_types.h"

static global_values_t *shared_info = NULL;

static void handle_application_health(void) {
	/* All errors are able to operate in fail-safe except for servo */
	bool error_present = false;
	error_e error_copy[NUM_THREADS] = { 0 };
	pthread_mutex_lock(&shared_info->mutex);
	(void)memccpy(error_copy, shared_info->thread_errors, NUM_THREADS, sizeof(error_e));
	pthread_mutex_unlock(&shared_info->mutex);

	for (uint8_t ii = 0U; ii < NUM_THREADS; ++ii) {
		if (error_copy[ii].is_set) {
			error_present = true;
			if (strncmp(THREAD_NAMES[ii], "vent_controller", 50U) == 0) {
				atomic_store(&shared_info->is_shutdown_requested, true);
			}
			LOG((thread_index_e)ii, "%s", error_copy[ii].error_msg);
		}
	}
	atomic_store(&shared_info->application_is_in_error, error_present);
	sleep(2U);
}

void *state_management_thread_entry(void *arg) {
	LOG(STATE_MANAGEMENT, "Starting state management thread");
	shared_info = (global_values_t *)arg;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		handle_application_health();
		increment_heartbeat(shared_info, STATE_MANAGEMENT);
	}
	LOG(STATE_MANAGEMENT, "Shutting down state management thread");
	return NULL;
}
