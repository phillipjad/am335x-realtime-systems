#include "led.h"
#include <unistd.h>

/* Local project includes after system libraries */
#include "heartbeat.h"
#include "logger.h"
#include "project_types.h"

static global_values_t *shared_info = NULL;

void *led_thread_entry(void *arg) {
	LOG("Starting LED thread");
	shared_info = (global_values_t *)arg;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		sleep(2U);
		increment_heartbeat(shared_info, LED);
	}
	LOG("Shutting down LED thread");
	return NULL;
}
