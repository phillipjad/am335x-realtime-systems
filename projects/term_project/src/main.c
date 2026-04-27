#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <unistd.h>

/* Local project includes after system libraries */
#ifdef USE_CONFIG
#include "app_config.h"
#endif /* USE_CONFIG */
#include "project_constants.h"
#ifndef USE_CONFIG
#include "user_input.h"
#include <string.h> /* string.h needed for memset */
#endif              /* !USE_CONFIG */
#include "gpio_control.h"
#include "lcd_screen.h"
#include "led.h"
#include "log_handler.h"
#include "logger.h"
#include "potentiometer.h"
#include "project_types.h"
#include "servo_controller.h"
#include "signal_handler.h"
#include "state_management.h"
#include "temperature_sensor.h"
#include "vent_control.h"

/* SCHED_FIFO priorities — RMS by period with exceptions noted below
 * Temp sensor is elevated above its 2s period to protect bit-timing during reads */
#define SCHED_PRI_LED (60)
#define SCHED_PRI_STATE_MGMT (59)
#define SCHED_PRI_TEMP_SENSOR (55)
#define SCHED_PRI_LCD (50)
#define SCHED_PRI_POTMETER (49)
#define SCHED_PRI_VENT (40)
#define SCHED_PRI_LOG_HANDLER (30)

/* Main controls all threads. As such, we register all of them in one neat struct for easier handling */
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

global_values_t shared_info = { 0 };
static project_threads_t threads = { 0 };

/*--------------------------------------
 * Static Function: application_init
 *--------------------------------------*/
static void application_init(void) {
	int32_t result = register_signal_handlers(&shared_info.is_shutdown_requested);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to register application signal handlers. Status code: %d", result);
	}
	LOG(NUM_THREADS, "Initialized application");
}

/*--------------------------------------
 * Static Function: hardware_init
 *--------------------------------------*/
static void hardware_init(void) {
	LOG(NUM_THREADS, "Initializing mmap");
	gpio_map_init();
	configuration_items_t *user_config = &shared_info.config;
	/* Init system LEDs */
	gpio_set_direction(user_config->gpio_layout.target_temp_led, GPIO_OUT);
	gpio_set_direction(user_config->gpio_layout.system_ok_led, GPIO_OUT);
	// Start with target reached off and system ok on
	gpio_set(user_config->gpio_layout.target_temp_led, GPIO_LOW);
	gpio_set(user_config->gpio_layout.system_ok_led, GPIO_HIGH);
	LOG(NUM_THREADS, "Initialized hardware LEDs");

	/* We'll start with the temp sensor set to output and high. Temp sensor thread will manipulate as necessary after */
	gpio_set_direction(user_config->gpio_layout.temp_sensor, GPIO_OUT);
	gpio_set(user_config->gpio_layout.temp_sensor, GPIO_HIGH);
	LOG(NUM_THREADS, "Initialized temp sensor");


	servo_init(user_config->gpio_layout.servo.servo_chip, user_config->gpio_layout.servo.servo_channel);
	LOG(NUM_THREADS, "Initialized servo");

	char i2c_path[USER_INPUT_MAX_LEN + 1U] = { 0 };
	(void)snprintf(i2c_path, USER_INPUT_MAX_LEN, "/dev/i2c-%u", user_config->gpio_layout.lcd_i2c_bus);
	// Only i2c-# value allowed is 2, so address will be 0x27
	user_config->gpio_layout.lcd_fd = lcd_init(i2c_path, 0x27);
	LOG(NUM_THREADS, "Initialized LCD");

	potentiometer_init(user_config->gpio_layout.potentiometer);
	LOG(NUM_THREADS, "Initialized potentiometer");
}

/*--------------------------------------
 * Static Function: globals_init
 *--------------------------------------*/
static void globals_init(void) {
	LOG(NUM_THREADS, "Initializing global states");
	pthread_mutex_init(&shared_info.mutex, NULL);
	pthread_cond_init(&shared_info.cv, NULL);
	shared_info.current_state = STATE_IDLE;
	shared_info.servo_activation_time = (struct timespec){ 0 };
	shared_info.potentiometer_percentage_closed = 0;
}

/*--------------------------------------
 * Static Function: log_mode
 *--------------------------------------*/
static void log_mode(void) {
	LOG(NUM_THREADS, "RELEASE MODE ENABLED");
}

#ifndef USE_CONFIG
/*--------------------------------------
 * Static Function: get_user_configuration_items
 *--------------------------------------*/
static void get_user_configuration_items(configuration_items_t *user_config) {
	char input_buffer[USER_INPUT_MAX_LEN + 1U] = { 0 };

	LOG(NUM_THREADS, "Prompting user for configuration items");

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
#endif /* !USE_CONFIG */

static void make_rt_attr(pthread_attr_t *attr, int32_t priority) {
	struct sched_param sp = { .sched_priority = priority };
	if (pthread_attr_init(attr) != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to init thread attr");
	}
	if (pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED) != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to set thread attr inherit sched");
	}
	if (pthread_attr_setschedpolicy(attr, SCHED_FIFO) != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to set thread attr sched policy");
	}
	if (pthread_attr_setschedparam(attr, &sp) != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to set thread attr sched param");
	}
}

static void make_project_thread(pthread_t *thread, void *(*entry)(void *), void *arg, int32_t priority, const char *name) {
	pthread_attr_t attr;
	make_rt_attr(&attr, priority);
	int32_t result = pthread_create(thread, &attr, entry, arg);
	if (pthread_attr_destroy(&attr) != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to destroy thead attr");
	}
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to create %s thread", name);
	}
}

static void start_project_threads(void) {
	make_project_thread(&threads.vent_control_thread, &vent_control_thread_entry, (void *)&shared_info, SCHED_PRI_VENT,
	    THREAD_NAMES[VENT_CONTROL]);
	make_project_thread(&threads.log_handler_thread, &log_handler_thread_entry, (void *)&shared_info,
	    SCHED_PRI_LOG_HANDLER, THREAD_NAMES[LOG_HANDLER]);
	make_project_thread(&threads.lcd_screen_thread, &lcd_screen_thread_entry, (void *)&shared_info, SCHED_PRI_LCD,
	    THREAD_NAMES[LCD_SCREEN]);
	make_project_thread(&threads.temperature_sensor_thread, &temperature_sensor_thread_entry, (void *)&shared_info,
	    SCHED_PRI_TEMP_SENSOR, THREAD_NAMES[TEMP_SENSOR]);
	make_project_thread(&threads.led_thread, &led_thread_entry, (void *)&shared_info, SCHED_PRI_LED, THREAD_NAMES[LED]);
	make_project_thread(&threads.potentiometer_thread, &potentiometer_thread_entry, (void *)&shared_info,
	    SCHED_PRI_POTMETER, THREAD_NAMES[POTENTIOMETER]);
	make_project_thread(&threads.state_management_thread, &state_management_thread_entry, (void *)&shared_info,
	    SCHED_PRI_STATE_MGMT, THREAD_NAMES[STATE_MANAGEMENT]);
}

static void join_project_threads() {
	/* Join all producer threads first so the log_handler can drain their final messages */
	int32_t result = pthread_join(threads.vent_control_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG(NUM_THREADS, "Failed to join vent control thread");
	}
	result = pthread_join(threads.lcd_screen_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG(NUM_THREADS, "Failed to join LCD screen thread");
	}
	result = pthread_join(threads.temperature_sensor_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG(NUM_THREADS, "Failed to join temperature sensor thread");
	}
	result = pthread_join(threads.led_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG(NUM_THREADS, "Failed to join LED thread");
	}
	result = pthread_join(threads.potentiometer_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG(NUM_THREADS, "Failed to join potentiometer thread");
	}
	result = pthread_join(threads.state_management_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG(NUM_THREADS, "Failed to join state management thread");
	}

	/* Wake the log_handler so it exits its wait promptly and drains remaining messages */
	(void)pthread_cond_broadcast(&shared_info.logger.log_cv);

	result = pthread_join(threads.log_handler_thread, NULL);
	if (result != STATUS_SUCCESS) {
		LOG(NUM_THREADS, "Failed to join log handler thread");
	}
}

/**
 * @brief Shuts down the application and handles any required cleanup
 */
static void handle_shutdown(int32_t exit_code) {
	/* We should unconditionally set shutdown requested here in case we shutdown due to missed heartbeats */
	atomic_store(&shared_info.is_shutdown_requested, true);
	join_project_threads();
	LOG(NUM_THREADS, "Shutting down...");
	servo_shutdown();
	gpio_map_close();
	exit(exit_code);
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
	LOG(NUM_THREADS, "System information: %s, %s, %s, %s", sys_info.sysname, sys_info.release, sys_info.version,
	    sys_info.machine);
}

static void handle_critical_heartbeat_failure(void) {
	pthread_mutex_lock(&shared_info.mutex);
	shared_info.current_state = STATE_FAIL;
	pthread_mutex_unlock(&shared_info.mutex);
	struct timespec lcd_update = { .tv_sec = 1L, .tv_nsec = 0L };
	nanosleep(&lcd_update, NULL);
	atomic_store(&shared_info.is_shutdown_requested, true);
}

static int32_t check_heartbeats(void) {
	static uint64_t prev_heartbeats[NUM_THREADS] = { 0 };
	static uint8_t num_missed_heartbeats[NUM_THREADS] = { 0 };
	static const uint8_t MAX_MISSED_HEARTBEATS = 5U;
	for (uint8_t ii = 0U; ii < NUM_THREADS; ++ii) {
		if (atomic_load(&shared_info.is_shutdown_requested)) {
			return STATUS_SUCCESS;
		}
		if (shared_info.heartbeats[ii] <= prev_heartbeats[ii]) {
			++num_missed_heartbeats[ii];
			LOG(NUM_THREADS, "%s thread missed heartbeat %u time(s) in a row", THREAD_NAMES[ii], num_missed_heartbeats[ii]);
			if (num_missed_heartbeats[ii] >= MAX_MISSED_HEARTBEATS) {
				LOG(NUM_THREADS, "%s thread has stalled or deadlocked. Heartbeat missed %u times in a row",
				    THREAD_NAMES[ii], num_missed_heartbeats[ii]);
				handle_critical_heartbeat_failure();
				return STATUS_FAIL;
			}
		} else {
			prev_heartbeats[ii] = shared_info.heartbeats[ii];
			num_missed_heartbeats[ii] = 0U;
		}
	}

	return STATUS_SUCCESS;
}

static void verify_sudo(void) {
	if (geteuid() != 0) {
		LOG_AND_EXIT("This program must be run with sudo/root privileges");
	}
}

/* Application entrypoint */
int32_t main(void) {
	/* Initialize the system logger */
	init_log_handler(&shared_info);

	verify_sudo();
	configuration_items_t *user_config = &shared_info.config;
#ifdef USE_CONFIG
	load_app_config(user_config);
#endif /* USE_CONFIG */

	/* Log the mode that the binary was compiled with */
	log_mode();

	log_system_info();

	/* If initialization fails we fail-fast so no need for a return value */
	application_init();

#ifndef USE_CONFIG
	get_user_configuration_items(user_config);
#endif /* !USE_CONFIG */

	hardware_init();

	/* Global params init */
	globals_init();
	start_project_threads();
	int32_t exit_code = 0;
	while (!atomic_load(&shared_info.is_shutdown_requested)) {
		struct timespec heartbeat_check_ticker = { .tv_sec = 5L, .tv_nsec = 0L };
		nanosleep(&heartbeat_check_ticker, NULL);
		exit_code = check_heartbeats();
	}
	LOG(NUM_THREADS, "Starting application shutdown sequence...");

	pthread_mutex_lock(&shared_info.mutex);
	bool system_error = (shared_info.current_state == STATE_FAIL) || (shared_info.current_state == STATE_FAIL_SAFE);
	pthread_mutex_unlock(&shared_info.mutex);
	exit_code = (exit_code == STATUS_FAIL) ? exit_code : (system_error) ? (STATUS_FAIL) : exit_code;

	handle_shutdown(exit_code);
}
