#include <pthread.h>
#include <unistd.h>

/* Local project includes after system libraries */
#ifdef USE_CONFIG /* We only need this header if we are using config file logic */
#include "app_config.h"
#else /* If we aren't using a config file then we need user input */
#include "user_input.h"
#include <string.h> /* string.h needed for memset */
#endif              /* USE_CONFIG */
#ifdef NDEBUG       /* We only need this header when using mmap logic */
#include "gpio_control.h"
#endif
#include "gate_control.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "sensor_monitoring.h"
#include "signal_handler.h"
#include "supervisor_input.h"
#include "warning_light.h"

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

#ifdef NDEBUG
/*--------------------------------------
 * Static Function: hardware_init
 *--------------------------------------*/
static void hardware_init(void) {
	LOG("Initializing mmap");
	gpio_map_init();
	LOG("Initialized hardware buttons");
	configuration_items_t *user_config = &shared_info.config;
	gpio_set_direction(user_config->gpio_layout.east_button, GPIO_IN);
	gpio_set_direction(user_config->gpio_layout.west_button, GPIO_IN);

	LOG("Initialized hardware LEDs");
	gpio_set_direction(user_config->gpio_layout.led_1, GPIO_OUT);
	gpio_set_direction(user_config->gpio_layout.led_2, GPIO_OUT);
	// Start with lights off
	gpio_set(user_config->gpio_layout.led_1, false);
	gpio_set(user_config->gpio_layout.led_2, false);

	// TODO: NEED TO SETUP SERVO STILL
}
#endif

/*--------------------------------------
 * Static Function: globals_init
 *--------------------------------------*/
static void globals_init(void) {
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

#ifndef USE_CONFIG
/*--------------------------------------
 * Static Function: get_user_configuration_items
 *--------------------------------------*/
static void get_user_configuration_items(configuration_items_t *user_config) {
	char input_buffer[USER_INPUT_MAX_LEN + 1U] = { 0 };

	LOG("Prompting user for configuration items");

	/* East Button Pin */
	int32_t result = get_user_input(input_buffer, USER_INPUT_MAX_LEN, "What pin should be used for East Button");
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to get user input for East Button pin");
	}
	result = parse_input_to_uint8(input_buffer, &user_config->gpio_layout.east_button);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to parse user input for East Button GPIO");
	}
	(void)memset((void *)input_buffer, 0, (USER_INPUT_MAX_LEN + 1U));
	/* West Button Pin */
	result = get_user_input(input_buffer, USER_INPUT_MAX_LEN, "What pin should be used for West Button");
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to get user input for West Button pin");
	}
	result = parse_input_to_uint8(input_buffer, &user_config->gpio_layout.west_button);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to parse user input for West Button GPIO");
	}
	(void)memset((void *)input_buffer, 0, (USER_INPUT_MAX_LEN + 1U));
	/* Led 1 Pin */
	result = get_user_input(input_buffer, USER_INPUT_MAX_LEN, "What pin should be used for LED 1");
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to get user input for LED 1 pin");
	}
	result = parse_input_to_uint8(input_buffer, &user_config->gpio_layout.led_1);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to parse user input for LED 1 GPIO");
	}
	(void)memset((void *)input_buffer, 0, (USER_INPUT_MAX_LEN + 1U));
	/* Led 2 Pin */
	result = get_user_input(input_buffer, USER_INPUT_MAX_LEN, "What pin should be used for LED 2");
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to get user input for LED 2 pin");
	}
	result = parse_input_to_uint8(input_buffer, &user_config->gpio_layout.led_2);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to parse user input for LED 2 GPIO");
	}
	(void)memset((void *)input_buffer, 0, (USER_INPUT_MAX_LEN + 1U));
	/* Servo Pin */
	result = get_user_input(input_buffer, USER_INPUT_MAX_LEN, "What pin should be used for Servo (ex: 2b for EHRPWM2B)");
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to get user input for Servo pin");
	}
	result = parse_pwm_input(input_buffer, &user_config->gpio_layout.servo.chip, &user_config->gpio_layout.servo.channel);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to parse user input for Servo EHRPWM pin");
	}
}
#endif /* USE_CONFIG */

static void handle_shutdown(void) {
	LOG("Shutting down...");
#ifdef NDEBUG
	/* Hardware mmap close if we are in release mode */
	gpio_map_close();
#endif
	exit(EXIT_SUCCESS);
}

/* Application entrypoint */
int32_t main(void) {
	configuration_items_t *user_config = &shared_info.config;
#ifdef USE_CONFIG /* If we are using a config file then we load it in first */
	load_app_config(user_config);
#endif /* USE_CONFIG */

	/* Log the mode that the binary was compiled with */
	log_mode();

	/* If initialization fails we fail-fast so no need for a return value */
	application_init();

#ifndef USE_CONFIG /* If we are not using config we should get inputs from stdio */
	get_user_configuration_items(user_config);
#endif /* USE_CONFIG */

/* Hardware mmap init if we are in release mode */
#ifdef NDEBUG
	hardware_init();
#endif

	/* Global params init */
	globals_init();
	/* Start threads */
	pthread_t sensor_monitoring_thread = { 0 };
	pthread_t supervisor_input_thread = { 0 };
	/*
	  int32_t result = pthread_create(&gate_control_thread, NULL, &gate_control_thread_entry, (void *)&shared_info);
	  if (result != STATUS_SUCCESS) {
	      LOG_AND_EXIT("Failed to create gate control thread");
	  }
	  result = pthread_create(&warning_light_thread, NULL, &warning_light_thread_entry, (void *)&shared_info);
	  if (result != STATUS_SUCCESS) {
	      LOG_AND_EXIT("Failed to create warning light thread");
	  }
	*/
	int32_t result = pthread_create(&sensor_monitoring_thread, NULL, &sensor_monitoring_thread_entry, (void *)&shared_info);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create sensor monitoring thread");
	}

	result = pthread_create(&supervisor_input_thread, NULL, &supervisor_input_thread_entry, (void *)&shared_info);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create supervisor input thread");
	}

	/*
	  result = pthread_join(gate_control_thread, NULL);
	  if (result != STATUS_SUCCESS) {
	      LOG("Failed to join gate control thread");
	  }
	  result = pthread_join(warning_light_thread, NULL);
	  if (result != STATUS_SUCCESS) {
	      LOG("Failed to join warning light thread");
	  }
	*/
	result = pthread_join(sensor_monitoring_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG("Failed to join sensor monitoring thread");
	}

	result = pthread_join(supervisor_input_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG("Failed to join supervisor input thread");
	}

	LOG("Starting application shutdown sequence...");

	handle_shutdown();
}
