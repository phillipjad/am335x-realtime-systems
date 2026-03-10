#include <pthread.h>
#include <string.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "gate_control/gate_control.h"
#include "sensor_monitoring/sensor_monitoring.h"
#include "utils/app_config.h"
#include "utils/gpio_control.h"
#include "utils/logger.h"
#include "utils/project_constants.h"
#include "utils/project_types.h"
#include "utils/signal_handler.h"
#include "utils/user_input.h"
#include "warning_light/warning_light.h"

#define USER_INPUT_MAX_LEN (1024U)

extern configuration_items_t user_config;
global_values_t shared_info = { 0 };

/*--------------------------------------
 * Static Function: application_init
 *--------------------------------------*/
static void application_init(void) {
	int32_t result = register_signal_handlers(&shared_info.is_shutdown_requested);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to register application signal handlers. Status code: %d", result);
	}
	LOG("Initialized application");
}

/*--------------------------------------
 * Static Function: hardware_init
 *--------------------------------------*/
static void hardware_init(void) {
	LOG("Initializing mmap");
	gpio_map_init();
	LOG("Initialized hardware buttons");
	gpio_set_direction(user_config.gpio_layout.east_button, GPIO_IN);
	gpio_set_direction(user_config.gpio_layout.west_button, GPIO_IN);

	LOG("Initialized hardware LEDs");
	gpio_set_direction(user_config.gpio_layout.led_1, GPIO_OUT);
	gpio_set_direction(user_config.gpio_layout.led_2, GPIO_OUT);
	// Start with lights off
	gpio_set(user_config.gpio_layout.led_1, false);
	gpio_set(user_config.gpio_layout.led_2, false);

	// TODO: NEED TO SETUP SERVO STILL
}

/*--------------------------------------
 * Static Function: software_init
 *--------------------------------------*/
static void software_init(void) {
	LOG("Initializing global states");
	pthread_mutex_init(&shared_info.mutex, NULL);
	pthread_cond_init(&shared_info.cv, NULL);
	shared_info.current_state = STATE_IDLE;
	shared_info.current_direction = DIRECTION_NONE;
	shared_info.arrival_time = (struct timespec){ 0 };
	shared_info.clear_time = (struct timespec){ 0 };
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

/* Hardware mmap init if we are in release mode */
#ifdef NDEBUG
	hardware_init();
#endif

	/* Global params init */
	software_init();
	/* Start threads */
	pthread_t sensor_monitoring_thread = { 0 };
	int32_t result = pthread_create(&gate_control_thread, NULL, &gate_control_thread_entry, (void *)&shared_info);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create gate control thread");
	}
	result = pthread_create(&warning_light_thread, NULL, &warning_light_thread_entry, (void *)&shared_info);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create warning light thread");
	}
	result = pthread_create(&sensor_monitoring_thread, NULL, &sensor_monitoring_thread_entry, (void *)&shared_info);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create sensor monitoring thread");
	}

	result = pthread_join(gate_control_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG("Failed to join gate control thread");
	}
	result = pthread_join(warning_light_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG("Failed to join warning light thread");
	}
	result = pthread_join(sensor_monitoring_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG("Failed to join sensor monitoring thread");
	}

	LOG("Starting application shutdown sequence...");

/* Hardware mmap close if we are in release mode */
#ifdef NDEBUG
	gpio_map_close();
#endif

	// handle_shutdown();
}
