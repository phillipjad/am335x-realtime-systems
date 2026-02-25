#include "io_logic.h"

#include <fcntl.h>
#include <stdio.h>

#include "logger.h"
#include "project_constants.h"

/** Path for sysfs GPIO control */
#define SYSFS_GPIO_PATH ("/sys/class/gpio")
/** Path for GPIO export */
#define SYSFS_GPIO_EXPORT_PATH ("/sys/class/gpio/export")
/** Maximum buffer size for sysfs gpio-related writing and reading */
#define SYSFS_GPIO_MAX_BUFFER_SIZE (64U)
/** Out direction for GPIO */
#define SYSFS_GPIO_DIR_OUT ("out")

static void write_to_file(const char *path, const char *value) {
	/* If anything in this function fails then we should exit catastrophically */
	FILE *fp = fopen(path, "w");
	if (fp == NULL) {
		LOG_AND_EXIT("Failed to open file at (%s) during write attempt", path);
	}
	int32_t result = fprintf(fp, value);
	if (result < 0) {
		LOG_AND_EXIT("Failed to write %s to %s", value, path);
	}
	result = fclose(fp);
	if (result < 0) {
		LOG_AND_EXIT("Failed to close file at %s", path);
	}
}

static void export_gpio(uint8_t gpio) {
	char gpio_pin_as_string[SYSFS_GPIO_MAX_BUFFER_SIZE + 1U] = { 0 };
	(void)snprintf(gpio_pin_as_string, SYSFS_GPIO_MAX_BUFFER_SIZE, "%u", gpio);
	write_to_file(SYSFS_GPIO_EXPORT_PATH, gpio_pin_as_string);
}

static void set_gpio_dir(uint8_t gpio, const char *dir) {
	char gpio_direction_path[MAX_FILE_PATH_LENGTH + 1U] = { 0 };
	(void)snprintf(gpio_direction_path, MAX_FILE_PATH_LENGTH, "%s/gpio%u/direction", SYSFS_GPIO_PATH, gpio);
	write_to_file(gpio_direction_path, dir);
}

static void gpio_write(uint8_t gpio, int8_t value) {
	char gpio_path[MAX_FILE_PATH_LENGTH + 1U] = { 0 };
	(void)snprint(gpio_path, MAX_FILE_PATH_LENGTH, "%s/gpio%u/value", SYSFS_GPIO_PATH, gpio);
	char value_as_string[SYSFS_GPIO_MAX_BUFFER_SIZE + 1U] = { 0 };
	(void)snprintf(value_as_string, SYSFS_GPIO_MAX_BUFFER_SIZE, "%d", value);
	write_to_file(gpio_path, value_as_string);
}

void signal_gpio(uint8_t gpio_pin, int8_t value) {
	export_gpio(gpio_pin);
	set_gpio_dir(gpio_pin, SYSFS_GPIO_DIR_OUT);
	gpio_write(gpio_pin, value);
}
