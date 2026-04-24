#include "signal_handler.h"

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "logger.h"

atomic_bool *shutdown_requested = NULL;

/*--------------------------------------
 * Static Function: signal_handler
 *--------------------------------------*/
static void signal_handler(int32_t signal_number) {
	(void)signal_number;
	LOG(NUM_THREADS, "Setting shutdown flag!");
	atomic_store(shutdown_requested, true);
}

/*--------------------------------------
 * Function: register_signal_handlers
 *--------------------------------------*/
int32_t register_signal_handlers(atomic_bool *is_shutdown_requested) {
	shutdown_requested = is_shutdown_requested;
	struct sigaction sig_handler = { 0 };

	sig_handler.sa_handler = signal_handler;
	sigemptyset(&sig_handler.sa_mask);
	sig_handler.sa_flags = 0;

	if (sigaction(SIGINT, &sig_handler, NULL) == -1) {
		return STATUS_FAIL;
	}

	if (sigaction(SIGTERM, &sig_handler, NULL) == -1) {
		return STATUS_FAIL;
	}

	return STATUS_SUCCESS;
}
