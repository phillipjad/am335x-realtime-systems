#include <string.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "signal_handler.h"
#include "traffic_logic.h"
#include "user_input.h"

#define USER_INPUT_MAX_LEN (1024U)

#define NUM_UTILIZED_PINS (6U)

#define SEC_PER_MINUTE (60U)

/* Struct for parsing user input for pins */
typedef struct {
	char *pin_string;
	uint8_t *pin_pointer;
} pin_io_struct_t;

static configuration_items_t user_config = { 0 };

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
#if defined(DEBUG)
	LOG("DEBUG MODE ENABLED");
#elif defined(NDEBUG)
	LOG("RELEASE MODE ENABLED");
#else
	LOG_AND_EXIT("No release mode detected!");
#endif
}

/*--------------------------------------
 * Static Function: get_user_configuration_items
 *--------------------------------------*/
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
	if ((temp < 0.0) || (temp > (float64_t)UINT16_MAX)) {
		LOG_AND_EXIT("Green light duration too large");
	}
	user_config.green_light_duration_s = (uint16_t)temp;
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to parse green light duration string to int");
	}
	(void)memset(input_buffer, 0, sizeof(input_buffer));

#ifdef NDEBUG
	/* If built in RELEASE mode then we should get user input for pin layout from stdio */
	gpio_pin_layout_t *pin_layout = &user_config.pin_layout;
	pin_io_struct_t pin_info[NUM_UTILIZED_PINS] = { { .pin_string = "North/South green light",
		                                            .pin_pointer = &pin_layout->green_light_ns },
		{ .pin_string = "North/South yellow light", .pin_pointer = &pin_layout->yellow_light_ns },
		{ .pin_string = "North/South red light", .pin_pointer = &pin_layout->red_light_ns },
		{ .pin_string = "East/West green light", .pin_pointer = &pin_layout->green_light_ew },
		{ .pin_string = "East/West yellow light", .pin_pointer = &pin_layout->yellow_light_ew },
		{ .pin_string = "East/West red light", .pin_pointer = &pin_layout->red_light_ew } };
	for (uint8_t ii = 0U; ii < NUM_UTILIZED_PINS; ++ii) {
		pin_io_struct_t *io_option = &pin_info[ii];
		char prompt[255] = { 0 };
		(void)snprintf(prompt, 255, "Which pin should be used for %s", io_option->pin_string);
		result = get_user_input(input_buffer, USER_INPUT_MAX_LEN, prompt);
		if (result != STATUS_SUCCESS) {
			LOG_AND_EXIT("Failed to get user input for %s pin", io_option->pin_string);
		}
		result = parse_input_to_uint8(input_buffer, io_option->pin_pointer);
		if (result != STATUS_SUCCESS) {
			LOG_AND_EXIT("Failed to parse user input for %s pin", pin_info->pin_string);
		}
		(void)memset(input_buffer, 0, sizeof(input_buffer));
	}
#endif /* NDEBUG */
}

/* Application entrypoint */
int32_t main(void) {
	/* Log the mode that the binary was compiled with */
	log_mode();

	/* If initialization fails we fail-fast so no need for a return value */
	application_init();

	get_user_configuration_items();

	while (is_shutdown_requested() == 0) {
		run_traffic_signal();
		(void)sleep(MAIN_THREAD_SLEEP_S);
	}

	LOG("Starting application shutdown sequence...");
	handle_shutdown();
}
