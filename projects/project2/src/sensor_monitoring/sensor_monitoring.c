#include "sensor_monitoring.h"

/* Local project includes after system libraries */
#include "logger.h"
#include "project_types.h"

/*--------------------------------------
 * Function: sensor_monitoring_thread_entry
 *--------------------------------------*/
void *sensor_monitoring_thread_entry(void *arg) {
	global_values_t *shared_info = (global_values_t *)arg;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		LOG("Hello from sensor_monitoring!");
		struct timespec timer = { 0 };
		timer.tv_sec = 3;
		nanosleep(&timer, NULL);
	}

	LOG("Shutting down sensor monitoring thread");
	return NULL;
}
