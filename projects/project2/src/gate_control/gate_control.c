#include "gate_control.h"

/* Local project includes after system libraries */
#include "logger.h"
#include "project_types.h"

/*--------------------------------------
 * Function: gate_control_thread_entry
 *--------------------------------------*/
void *gate_control_thread_entry(void *arg) {
	global_values_t *shared_info = (global_values_t *)arg;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		struct timespec timer = { 0 };
		timer.tv_sec = 3;
		nanosleep(&timer, NULL);
	}

	LOG("Shutting down gate control thread");
	return NULL;
}
