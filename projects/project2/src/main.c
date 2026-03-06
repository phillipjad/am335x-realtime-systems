#include <string.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "app_config.h"
#include "logger.h"
#include "signal_handler.h"
#include "user_input.h"

extern configuration_items_t user_config;

#define USER_INPUT_MAX_LEN (1024U)

configuration_items_t user_config = { 0 };

/*--------------------------------------
 * Static Function: application_init
 *--------------------------------------*/
static void application_init(void) {
	int32_t result = register_signal_handlers();
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to register application signal handlers. Status code: %d", result);
	}
	LOG("Initialized application");
}

/*--------------------------------------
 * Static Function: log_mode
 *--------------------------------------*/
static void log_mode(void) {
#ifdef DEBUG
	LOG("DEBUG MODE ENABLED");
#else
#ifdef NDEBUG
	LOG("RELEASE MODE ENABLED");
#else
	LOG_AND_EXIT("No release mode detected!");
#endif
#endif
}

/*--------------------------------------
 * Static Function: get_user_configuration_items
 *--------------------------------------*/
#ifndef USE_CONFIG
static void get_user_configuration_items(void) {
	return;
}
#endif /* USE_CONFIG */

/* Application entrypoint */
int32_t main(void) {
#ifdef USE_CONFIG /* If we are using a config file then we load it in first */
	load_app_config(&user_config);
#endif /* USE_CONFIG */

	/* Log the mode that the binary was compiled with */
	log_mode();

	/* If initialization fails we fail-fast so no need for a return value */
	application_init();

#ifndef USE_CONFIG /* If we are not using config we should get inputs from stdio */
	get_user_configuration_items();
#endif /* USE_CONFIG */

	while (is_shutdown_requested() == 0) {
		LOG("Hello from P2!");
		(void)sleep(MAIN_THREAD_SLEEP_S);
	}
	LOG("Starting application shutdown sequence...");
	// handle_shutdown();
}
