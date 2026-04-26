#include "potentiometer.h"
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

/* Local project includes after system libraries */
#include "heartbeat.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

#define POT_INPUT_SIZE 20
#define POT_MAX_VALUE 4095.0
#define POT_MIN_VALUE 0.0

static global_values_t *shared_info = NULL;
static int fd = 0;

static void potentiometer_init_hw(uint8_t pin_number) {
	// Map pin number value to AIN #
	// 5 as default AIN value
	uint8_t ain_number = 5;
	switch (pin_number) {
	case 33: ain_number = 4; break;
	case 35: ain_number = 6; break;
	case 36: ain_number = 5; break;
	case 37: ain_number = 2; break;
	case 38: ain_number = 3; break;
	case 39: ain_number = 0; break;
	case 40: ain_number = 1; break;
	default: LOG_AND_EXIT("ERROR: Invalid pin number (%u) mapping to AIN # given in hardware init.", pin_number);
	}
	char potentiometer_path[MAX_FILE_PATH_LENGTH] = { 0 };
	(void)snprintf(potentiometer_path, sizeof(potentiometer_path), "/sys/bus/iio/devices/iio:device0/in_voltage%u_raw", ain_number);
	fd = open(potentiometer_path, O_RDONLY);
	if (fd < 0) {
		LOG_AND_EXIT("Failed to open %s for", potentiometer_path);
	}
	LOG(POTENTIOMETER, "Potentiometer initialized with AIN-%u", ain_number);
}

/*--------------------------------------
 * Function: potentiometer_init
 *--------------------------------------*/
void potentiometer_init(uint8_t pin_number) {
	potentiometer_init_hw(pin_number);
}

void *potentiometer_thread_entry(void *arg) {
	LOG(POTENTIOMETER, "Starting potentiometer thread");
	shared_info = (global_values_t *)arg;
	char input[POT_INPUT_SIZE] = { 0 };
	float64_t percent = 0.0;
	float64_t temp = 0;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		sleep(2U);
		// Reset fd to start
		lseek(fd, 0, SEEK_SET);
		(void)memset(input, 0, sizeof(input));
		// Read potentiometer input
		ssize_t result = read(fd, input, sizeof(input) - 1);
		pthread_mutex_lock(&shared_info->mutex);
		if (result < 0) {
			LOG(POTENTIOMETER, "Potentiometer read error: %s", strerror(errno));
			set_error(&shared_info->thread_errors[POTENTIOMETER], strerror(errno));
		} else {
			if (has_error(&shared_info->thread_errors[POTENTIOMETER])) {
				clear_error(&shared_info->thread_errors[POTENTIOMETER]);
			}
			float64_t value = atof(input);
			percent = value / POT_MAX_VALUE;
			temp = MIN_TEMP + ((MAX_TEMP - MIN_TEMP) * percent);
			// Change to percentage after adjusting temperature
			percent = percent * 100;

			// If in RUNNING or FAIL_SAFE mode, will read value and adjust target_temp or servo duty_cycle
			// Write to global potentiometer values
			shared_info->potentiometer_percentage_closed = percent;
			shared_info->target_temp = temp;
		}
		increment_heartbeat(shared_info, POTENTIOMETER);
		pthread_mutex_unlock(&shared_info->mutex);
	}
	close(fd);
	LOG(POTENTIOMETER, "Shutting down potentiometer thread");
	return NULL;
}
