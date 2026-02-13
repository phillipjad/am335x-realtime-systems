#include <unistd.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "signal_handler.h"
#include "traffic_logic.h"

static void application_init(void) {
	int32_t result = register_signal_handlers();
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to register application signal handlers. Status code: %d", result);
	}
	LOG("Initialized application");
}

int32_t main(void) {
#if defined(DEBUG)
	LOG("DEBUG MODE ENABLED");
#elif defined(NDEBUG)
	LOG("RELEASE MODE ENABLED");
#else
	LOG_AND_EXIT("No release mode detected!");
#endif

	/* If initialization fails we fail-fast so no need for a return value */
	application_init();

	while (is_shutdown_requested() == 0) {
		run_traffic_signal();
		(void)sleep(MAIN_THREAD_SLEEP_S);
	}

	LOG("Starting application shutdown sequence...");
	handle_shutdown();
}
