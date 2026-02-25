#include "io_logic.h"

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "logger.h"
#include "project_constants.h"

#ifdef USE_MMAP
#include "gpio_control.h"
#endif

#ifndef USE_MMAP /* Constants for sysfs implementation */
/** Path for sysfs GPIO control */
#define SYSFS_GPIO_PATH ("/sys/class/gpio")
/** Path for GPIO export */
#define SYSFS_GPIO_EXPORT_PATH ("/sys/class/gpio/export")
/** Path for GPIO unexport */
#define SYSFS_GPIO_UNEXPORT_PATH ("/sys/class/gpio/unexport")
/** Maximum buffer size for sysfs gpio-related writing and reading */
#define SYSFS_GPIO_MAX_BUFFER_SIZE (64U)
/** Out direction for GPIO */
/** The number of times to attempt to open GPIO file */
#define SYSFS_GPIO_MAX_FILE_POLL_ATTEMPTS (3U)
/* General constants */
#define GPIO_DIR_OUT ("out")
/** A quarter of a second represented in nanoseconds */
#define QUARTER_SECOND_AS_NSEC (250000000L)
#else /* Constants for mmap implementation */
#endif


#ifndef USE_MMAP /* If not compiled for mmap use sysfs */
/*--------------------------------------
 * Static Function: wait_for_file
 *--------------------------------------*/
static bool wait_for_file(const char *path) {
	FILE *fp = fopen(path, "w");
	uint8_t attempts = 0U;
	while ((fp == NULL) && (attempts < SYSFS_GPIO_MAX_FILE_POLL_ATTEMPTS)) {
		++attempts;
		/* If file still isn't accessible on disk, sleep and reattempt access */
		struct timespec timer = { 0 };
		timer.tv_nsec = QUARTER_SECOND_AS_NSEC;
		nanosleep(&timer, NULL);
		fp = fopen(path, "w");
	}
	if (fp != NULL) {
		(void)fclose(fp);
		return true;
	} else {
		return false;
	}
}

/*--------------------------------------
 * Static Function: write_to_file
 *--------------------------------------*/
static void write_to_file(const char *path, const char *value) {
	/* If anything in this function fails then we should exit catastrophically */
	FILE *fp = fopen(path, "w");
	if (fp == NULL) {
		LOG_AND_EXIT("Failed to open file at (%s) during write attempt", path);
	}
	int32_t result = fprintf(fp, "%s", value);
	if (result < 0) {
		LOG_AND_EXIT("Failed to write %s to %s", value, path);
	}
	result = fclose(fp);
	if (result < 0) {
		LOG_AND_EXIT("Failed to close file at %s", path);
	}
}

/*--------------------------------------
 * Static Function: export_gpio
 *--------------------------------------*/
static void export_gpio(uint8_t gpio) {
	char gpio_pin_as_string[SYSFS_GPIO_MAX_BUFFER_SIZE + 1U] = { 0 };
	(void)snprintf(gpio_pin_as_string, SYSFS_GPIO_MAX_BUFFER_SIZE, "%u", gpio);
	write_to_file(SYSFS_GPIO_EXPORT_PATH, gpio_pin_as_string);
}

/*--------------------------------------
 * Static Function: unexport_gpio
 *--------------------------------------*/
static void unexport_gpio(uint8_t gpio) {
	char gpio_pin_as_string[SYSFS_GPIO_MAX_BUFFER_SIZE + 1U] = { 0 };
	(void)snprintf(gpio_pin_as_string, SYSFS_GPIO_MAX_BUFFER_SIZE, "%u", gpio);
	/* We want to ignore any errors received here, this unexport is best effort just so exporting always succeeds */
	FILE *fp = fopen(SYSFS_GPIO_UNEXPORT_PATH, "w");
	if (fp != NULL) {
		(void)fprintf(fp, "%s", gpio_pin_as_string);
		(void)fclose(fp);
	}
}

/*--------------------------------------
 * Static Function: set_gpio_dir
 *--------------------------------------*/
static void set_gpio_dir(uint8_t gpio, const char *dir) {
	char gpio_direction_path[MAX_FILE_PATH_LENGTH + 1U] = { 0 };
	(void)snprintf(gpio_direction_path, MAX_FILE_PATH_LENGTH, "%s/gpio%u/direction", SYSFS_GPIO_PATH, gpio);
	/* Ensure that direction file exists */
	if (!wait_for_file(gpio_direction_path)) {
		LOG_AND_EXIT("Timed out waiting for direction file at %s", gpio_direction_path);
	}
	write_to_file(gpio_direction_path, dir);
}

/*--------------------------------------
 * Static Function: gpio_write
 *--------------------------------------*/
static void gpio_write(uint8_t gpio, int8_t value) {
	char gpio_path[MAX_FILE_PATH_LENGTH + 1U] = { 0 };
	(void)snprintf(gpio_path, MAX_FILE_PATH_LENGTH, "%s/gpio%u/value", SYSFS_GPIO_PATH, gpio);
	char value_as_string[SYSFS_GPIO_MAX_BUFFER_SIZE + 1U] = { 0 };
	(void)snprintf(value_as_string, SYSFS_GPIO_MAX_BUFFER_SIZE, "%d", value);
	write_to_file(gpio_path, value_as_string);
}
#endif

/*--------------------------------------
 * Function: signal_gpio
 *--------------------------------------*/
void signal_gpio(uint8_t gpio_pin, int8_t value) {
#ifdef USE_MMAP
	gpio_set_direction_out(gpio_pin);
	gpio_set(gpio_pin, value >= 1 ? true : false);
#else
	unexport_gpio(gpio_pin);
	export_gpio(gpio_pin);
	set_gpio_dir(gpio_pin, GPIO_DIR_OUT);
	gpio_write(gpio_pin, value);
#endif
}
