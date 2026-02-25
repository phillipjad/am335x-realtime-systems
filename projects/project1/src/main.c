#include <string.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "app_config.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "signal_handler.h"
#include "traffic_logic.h"
#include "user_input.h"
#ifdef USE_MMAP
#include "gpio_control.h"
#endif

#define USER_INPUT_MAX_LEN (1024U)

#ifndef USE_CONFIG /* These are only necessary for stdio configuration */
#define NUM_UTILIZED_GPIO (6U)

/* Struct for parsing user input for GPIO */
typedef struct {
	const char *gpio_string;
	uint8_t *gpio_pointer;
} gpio_struct_t;
#endif /* USE_CONFIG */

configuration_items_t user_config = { 0 };

/*--------------------------------------
 * Static Function: application_init
 *--------------------------------------*/
static void application_init(void) {
	int32_t result = register_signal_handlers();
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to register application signal handlers. Status code: %d", result);
	}
#ifdef USE_MMAP
	LOG("Init for GPIO mmap");
	gpio_map_init();
#endif
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
	char input_buffer[USER_INPUT_MAX_LEN] = { 0 };

	/* Get green light duration */
	int32_t result = get_user_input(input_buffer, USER_INPUT_MAX_LEN, "How many minutes should green lights last");
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to get user input for green light duration");
	}
	/* Parse user input as float in case of decimal input */
	float64_t temp = 0.0;
	result = parse_input_to_float64(input_buffer, &temp);
	temp *= (float64_t)SEC_PER_MINUTE;
	float64_t uint16_max_as_float = (float64_t)UINT16_MAX;
	if ((temp < 0.0) || (temp > uint16_max_as_float)) {
		LOG_AND_EXIT("Green light duration too large");
	}
	user_config.green_light_duration_s = (uint16_t)temp;
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to parse green light duration string to int");
	}
	(void)memset((void *)input_buffer, 0, sizeof(input_buffer));

#ifdef NDEBUG
	/* If built in RELEASE mode then we should get user input for gpio layout from stdio */
	gpio_layout_t *const gpio_layout = &user_config.gpio_layout;
	gpio_struct_t gpio_info[NUM_UTILIZED_GPIO] = { { .gpio_string = "North/South green light",
		                                           .gpio_pointer = &gpio_layout->green_light_ns },
		{ .gpio_string = "North/South yellow light", .gpio_pointer = &gpio_layout->yellow_light_ns },
		{ .gpio_string = "North/South red light", .gpio_pointer = &gpio_layout->red_light_ns },
		{ .gpio_string = "East/West green light", .gpio_pointer = &gpio_layout->green_light_ew },
		{ .gpio_string = "East/West yellow light", .gpio_pointer = &gpio_layout->yellow_light_ew },
		{ .gpio_string = "East/West red light", .gpio_pointer = &gpio_layout->red_light_ew } };
	for (uint8_t ii = 0U; ii < NUM_UTILIZED_GPIO; ++ii) {
		gpio_struct_t *io_option = &gpio_info[ii];
		char prompt[255] = { 0 };
		(void)snprintf(prompt, 255, "Which gpio should be used for %s", io_option->gpio_string);
		result = get_user_input(input_buffer, USER_INPUT_MAX_LEN, prompt);
		if (result != STATUS_SUCCESS) {
			LOG_AND_EXIT("Failed to get user input for %s gpio", io_option->gpio_string);
		}
		result = parse_input_to_uint8(input_buffer, io_option->gpio_pointer);
		if (result != STATUS_SUCCESS) {
			LOG_AND_EXIT("Failed to parse user input for %s gpio", io_option->gpio_string);
		}
		(void)memset((void *)input_buffer, 0, sizeof(input_buffer));
	}
#endif /* NDEBUG */
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

#ifndef USE_CONFIG /* If we are not using config we should get inpus from stdio */
	get_user_configuration_items();
#endif /* USE_CONFIG */

	while (is_shutdown_requested() == 0) {
		run_traffic_signal(user_config.green_light_duration_s);
		(void)sleep(MAIN_THREAD_SLEEP_S);
	}
	LOG("Starting application shutdown sequence...");
	handle_shutdown();
}
