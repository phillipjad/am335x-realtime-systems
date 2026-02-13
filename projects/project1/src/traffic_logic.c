#include "traffic_logic.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

void handle_shutdown(void) {
	/* TODO: This will need to actually handle shutdown at some point */
	uint32_t sleep_time = sleep(SHUTDOWN_DELAY_S);
	if (sleep_time == SHUTDOWN_DELAY_S) {
		LOG("Failed to gracefully shutdown");
	}
	LOG("Shutdown sequence successfully completed");
	exit(EXIT_SUCCESS);
}

void run_traffic_signal(void) {
	LOG("Green.");
	(void)sleep(1U);
	LOG("Yellow...");
	(void)sleep(1U);
	LOG("Red!");
	(void)sleep(1U);
}
