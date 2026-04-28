#include "fan_control.h"
#include <time.h>

/* Local project includes after system libraries */
#include "heartbeat.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

static global_values_t *shared_info = NULL;

void *fan_control_thread_entry(void *arg) {
	LOG(FAN_CONTROL, "Starting fan control thread");
	shared_info = (global_values_t *)arg;

	static const struct timespec thread_sleep = { .tv_sec = 1L, .tv_nsec = 0L };

	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		increment_heartbeat(shared_info, FAN_CONTROL);
		nanosleep(&thread_sleep, NULL);
	}

	LOG(FAN_CONTROL, "Shutting down fan control thread");
	return NULL;
}
