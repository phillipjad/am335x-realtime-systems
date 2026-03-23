#include "log_handler.h"
#include "logger.h"
#include "project_types.h"
#include <unistd.h>

static global_values_t *shared_info = NULL;

/*--------------------------------------
 * Function: log_handler_thread_entry
 *--------------------------------------*/
void *log_handler_thread_entry(void *arg) {
	shared_info = (global_values_t *)arg;
	LOG("Starting log handler thread!");
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		sleep(2U);
	}

	LOG("Shutting down log handler thread");
	return NULL;
}
