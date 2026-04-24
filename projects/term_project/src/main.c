#include <pthread.h>
#include <sys/utsname.h>

/* Local project includes after system libraries */
#ifdef USE_CONFIG /* We only need this header if we are using config file logic in release */
#ifdef NDEBUG
#include "app_config.h"
#endif             /* NDEBUG */
#endif             /* USE_CONFIG */
#include "project_constants.h"
#ifndef USE_CONFIG /* We only need user input when not using a config file in release */
#ifdef NDEBUG
#include "user_input.h"
#include <string.h> /* string.h needed for memset */
#endif              /* NDEBUG */
#endif              /* !USE_CONFIG */
#ifdef NDEBUG       /* We only need this header when using mmap logic */
#include "gpio_control.h"
#endif /* NDEBUG */
#include "lcd_screen.h"
#include "led.h"
#include "log_handler.h"
#include "logger.h"
#include "potentiometer.h"
#include "project_types.h"
#include "sensor_monitoring.h"
#include "servo_controller.h"
#include "signal_handler.h"
#include "state_management.h"
#include "temperature_sensor.h"
#include "vent_control.h"

global_values_t shared_info = { 0 };

typedef struct {
	pthread_t vent_control_thread;
	pthread_t log_handler_thread;
	pthread_t sensor_monitoring_thread;
	pthread_t supervisor_input_thread;
	pthread_t lcd_screen_thread;
	pthread_t temperature_sensor_thread;
	pthread_t led_thread;
	pthread_t potentiometer_thread;
	pthread_t state_management_thread;
} project_threads_t;

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
	configuration_items_t *user_config = &shared_info.config;
	LOG("Initialized hardware LEDs");
	gpio_set_direction(user_config->gpio_layout.target_temp_led, GPIO_OUT);
	gpio_set_direction(user_config->gpio_layout.system_ok_led, GPIO_OUT);

	// Start with target reached off and system ok on
	gpio_set(user_config->gpio_layout.target_temp_led, false);
	gpio_set(user_config->gpio_layout.system_ok_led, true);

	LOG("Initialized servo");
	servo_init(user_config->gpio_layout.servo.servo_chip, user_config->gpio_layout.servo.servo_channel);

	LOG("Initialized LCD");
	char i2c_path[USER_INPUT_MAX_LEN + 1U] = { 0 };
	(void)snprintf(i2c_path, USER_INPUT_MAX_LEN, "/dev/i2c-%u", user_config->gpio_layout.lcd_i2c_bus);
	// Only i2c-# value allowed is 2, so address will be 0x27
	user_config->gpio_layout.lcd_fd = lcd_init(i2c_path, 0x27);
}
#endif /* NDEBUG */

/*--------------------------------------
 * Static Function: globals_init
 *--------------------------------------*/
static void globals_init(void) {
	LOG("Initializing global states");
	pthread_mutex_init(&shared_info.mutex, NULL);
	pthread_cond_init(&shared_info.cv, NULL);
	shared_info.current_state = STATE_IDLE;
	shared_info.servo_activation_time = (struct timespec){ 0 };
	shared_info.servo_health = true;
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
#ifdef NDEBUG
/*--------------------------------------
 * Static Function: get_user_configuration_items
 *--------------------------------------*/
static void get_user_configuration_items(configuration_items_t *user_config) {
	char input_buffer[USER_INPUT_MAX_LEN + 1U] = { 0 };

	LOG("Prompting user for configuration items");

	/* target temperature status LED Pin */
	int32_t result = get_user_input(input_buffer, USER_INPUT_MAX_LEN, "What pin should be used for target temperature status LED");
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to get user input for target temperature status pin");
	}
	result = parse_input_to_uint8(input_buffer, &user_config->gpio_layout.target_temp_led);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to parse user input for target temperature status LED GPIO");
	}
	(void)memset((void *)input_buffer, 0, (USER_INPUT_MAX_LEN + 1U));
	/* system health LED Pin */
	result = get_user_input(input_buffer, USER_INPUT_MAX_LEN, "What pin should be used for system health LED");
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to get user input for system health LED pin");
	}
	result = parse_input_to_uint8(input_buffer, &user_config->gpio_layout.system_ok_led);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to parse user input for system health LED GPIO");
	}
	(void)memset((void *)input_buffer, 0, (USER_INPUT_MAX_LEN + 1U));
	/* Servo Pin */
	result = get_user_input(input_buffer, USER_INPUT_MAX_LEN, "What pin should be used for Servo (ex: 2b for EHRPWM2B)");
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to get user input for Servo pin");
	}
	result = parse_pwm_input(input_buffer, &user_config->gpio_layout.servo.servo_chip, &user_config->gpio_layout.servo.servo_channel);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to parse user input for Servo EHRPWM pin");
	}

	/* LCD Setup */
	result = get_user_input(input_buffer, USER_INPUT_MAX_LEN, "What I2C bus should be used for the LCD? (Ex: '2')");
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to get user input for LCD bus");
	}
	parse_input_to_uint8(input_buffer, &user_config->gpio_layout.lcd_i2c_bus);
	// Use I2C-2
	if (user_config->gpio_layout.lcd_i2c_bus != 2U) {
		LOG_AND_EXIT("Please use the I2C-2 bus");
	}
	(void)memset((void *)input_buffer, 0, (USER_INPUT_MAX_LEN + 1U));
	/* Servo Pin */
}
#endif /* NDEBUG */
#endif /* !USE_CONFIG */

/**
 * @brief Shuts down the application and handles any required cleanup
 */
static void handle_shutdown(void) {
	LOG("Shutting down...");
	servo_shutdown();
#ifdef NDEBUG
	/* Hardware mmap close if we are in release mode */
	gpio_map_close();
#endif /* NDEBUG */
	exit(EXIT_SUCCESS);
}

/**
 * @brief Logs the system machine name and architecture
 */
static void log_system_info(void) {
	struct utsname sys_info = { 0 };
	int32_t result = uname(&sys_info);
	if (result != EXIT_SUCCESS) {
		LOG_AND_EXIT("Failed to log system info");
	}
	LOG("System information: %s, %s, %s, %s", sys_info.sysname, sys_info.release, sys_info.version, sys_info.machine);
}

static void start_project_threads(project_threads_t *threads) {
	int32_t result = pthread_create(&threads->vent_control_thread, NULL, &vent_control_thread_entry, (void *)&shared_info);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create vent control thread");
	}
	result = pthread_create(&threads->log_handler_thread, NULL, &log_handler_thread_entry, (void *)&shared_info);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create log handler thread");
	}
	result = pthread_create(&threads->sensor_monitoring_thread, NULL, &sensor_monitoring_thread_entry, (void *)&shared_info);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create sensor monitoring thread");
	}
	result = pthread_create(&threads->lcd_screen_thread, NULL, &lcd_screen_thread_entry, (void *)&shared_info);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create LCD screen thread");
	}
	result = pthread_create(&threads->temperature_sensor_thread, NULL, &temperature_sensor_thread_entry, (void *)&shared_info);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create temperature sensor thread");
	}
	result = pthread_create(&threads->led_thread, NULL, &led_thread_entry, (void *)&shared_info);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create LED thread");
	}
	result = pthread_create(&threads->potentiometer_thread, NULL, &potentiometer_thread_entry, (void *)&shared_info);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create potentiometer thread");
	}
	result = pthread_create(&threads->state_management_thread, NULL, &state_management_thread_entry, (void *)&shared_info);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create state management thread");
	}
}

static void join_project_threads(project_threads_t *threads) {
	int32_t result = pthread_join(threads->vent_control_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG("Failed to join vent control thread");
	}
	result = pthread_join(threads->log_handler_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG("Failed to join log handler thread");
	}
	result = pthread_join(threads->sensor_monitoring_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG("Failed to join sensor monitoring thread");
	}
	result = pthread_join(threads->lcd_screen_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG("Failed to join LCD screen thread");
	}
	result = pthread_join(threads->temperature_sensor_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG("Failed to join temperature sensor thread");
	}
	result = pthread_join(threads->led_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG("Failed to join LED thread");
	}
	result = pthread_join(threads->potentiometer_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG("Failed to join potentiometer thread");
	}
	result = pthread_join(threads->state_management_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG("Failed to join state management thread");
	}
}

static void check_heartbeats() {
	uint64_t prev_heartbeats[NUM_THREADS] = { 0 };
	uint8_t num_missed_heartbeats[NUM_THREADS] = { 0 };
	char *thread_names[NUM_THREADS] = { "Vent controller", "Log handler", "Sensor monitoring", "Supervisor input",
		"LCD screen", "Temp sensor", "LED", "Potentiometer", "State management" };
	static const uint8_t MAX_MISSED_HEARTBEATS = 5U;
	for (uint8_t ii = 0U; ii < NUM_THREADS; ++ii) {
		if (shared_info.heartbeats[ii] <= prev_heartbeats[ii]) {
			++num_missed_heartbeats[ii];
			LOG("%s thread missed heartbeat %u time(s) in a row", thread_names[ii], num_missed_heartbeats[ii]);
			if (num_missed_heartbeats[ii] >= MAX_MISSED_HEARTBEATS) {
				// TODO: IMPLEMENT REAL DEADLOCK/STALL HANDLING LOGIC
				LOG("%s thread has stalled or deadlocked. Heartbeat missed %u times in a row", thread_names[ii],
				    num_missed_heartbeats[ii]);
				exit(1);
			}
		}
	}
}


/* Application entrypoint */
int32_t main(void) {
#ifdef NDEBUG /* Only need this in release */
	configuration_items_t *user_config = &shared_info.config;
#endif            /* NDEBUG */
#ifdef USE_CONFIG /* If we are using a config file then we load it in first */
#ifdef NDEBUG
	/* We don't configure anything in debug */
	load_app_config(user_config);
#endif /* NDEBUG */
#endif /* USE_CONFIG */

	/* Log the mode that the binary was compiled with */
	log_mode();

	log_system_info();

	/* If initialization fails we fail-fast so no need for a return value */
	application_init();

#ifndef USE_CONFIG /* If we are not using config we should get inputs from stdio */
#ifdef NDEBUG
	get_user_configuration_items(user_config);
#endif /* NDEBUG */
#endif /* !USE_CONFIG */

/* Hardware mmap init if we are in release mode */
#ifdef NDEBUG
	hardware_init();
#endif /* NDEBUG */

	/* Global params init */
	globals_init();

	project_threads_t threads = { 0 };
	start_project_threads(&threads);
	while (!atomic_load(&shared_info.is_shutdown_requested)) {
		struct timespec heartbeat_check_ticker = { .tv_sec = 5L, .tv_nsec = 0L };
		nanosleep(&heartbeat_check_ticker, NULL);
		check_heartbeats();
	}
	join_project_threads(&threads);

	LOG("Starting application shutdown sequence...");

	handle_shutdown();
}
