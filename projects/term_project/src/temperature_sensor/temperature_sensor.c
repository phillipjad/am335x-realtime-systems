#include "temperature_sensor.h"
#include <unistd.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_types.h"

static global_values_t *shared_info = NULL;

void *temperature_sensor_thread_entry(void *arg) {
	LOG("Starting temperature sensor thread");
	shared_info = (global_values_t *)arg;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		sleep(2U);
	}
	LOG("Shutting down temperature sensor thread");
	return NULL;
}
