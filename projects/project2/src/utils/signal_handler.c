#include "signal_handler.h"

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_types.h"

static volatile sig_atomic_t shutdown_requested = 0;

/*--------------------------------------
 * Static Function: signal_handler
 *--------------------------------------*/
static void signal_handler(int signal_number) {
	(void)signal_number;
	shutdown_requested = 1;
}

/*--------------------------------------
 * Function: is_shutdown_requested
 *--------------------------------------*/
int32_t is_shutdown_requested(void) {
	return shutdown_requested;
}

/*--------------------------------------
 * Function: register_signal_handlers
 *--------------------------------------*/
int32_t register_signal_handlers(void) {
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
