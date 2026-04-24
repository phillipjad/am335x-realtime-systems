#include "state_management.h"
#include <unistd.h>

/* Local project includes after system libraries */
#include "heartbeat.h"
#include "logger.h"
#include "project_types.h"

static global_values_t *shared_info = NULL;

void *state_management_thread_entry(void *arg) {
	LOG(STATE_MANAGEMENT, "Starting state management thread");
	shared_info = (global_values_t *)arg;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		sleep(2U);
		increment_heartbeat(shared_info, STATE_MANAGEMENT);
	}
	LOG(STATE_MANAGEMENT, "Shutting down state management thread");
	return NULL;
}
