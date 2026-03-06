#include "warning_light.h"

/* Local project includes after system libraries */
#include "logger.h"
#include "project_types.h"

/*--------------------------------------
 * Function: warning_light_thread_entry
 *--------------------------------------*/
void *warning_light_thread_entry(void *arg) {
	global_values_t *shared_info = (global_values_t *)arg;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		LOG("Hello from warning_light!");
		struct timespec timer = { 0 };
		timer.tv_sec = 3;
		nanosleep(&timer, NULL);
	}

	LOG("Shutting down warning light thread");
	return NULL;
}
