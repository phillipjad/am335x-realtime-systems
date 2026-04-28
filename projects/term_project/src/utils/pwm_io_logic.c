#include "pwm_io_logic.h"

#include <stdint.h>
#include <unistd.h>

#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

/** Maximum buffer size for sysfs pwm-related writing */
#define SYSFS_PWM_MAX_BUFFER_SIZE (64U)
/** PWM path */
#define PWM_CHIP_PATH "/sys/class/pwm"
/** The number of times to attempt to open PWM file */
#define SYSFS_PWM_MAX_FILE_POLL_ATTEMPTS (3U)
/** A quarter of a second represented in nanoseconds */
#define QUARTER_SECOND_AS_NSEC (250000000L)

/*--------------------------------------
 * Function: wait_for_file
 *--------------------------------------*/
static bool wait_for_file(const char *path) {
	bool file_exists = access(path, F_OK) == 0;
	uint8_t attempts = 0U;
	while ((!file_exists) && (attempts < SYSFS_PWM_MAX_FILE_POLL_ATTEMPTS)) {
		++attempts;
		/* If file still isn't accessible on disk, sleep and reattempt access */
		struct timespec timer = { 0 };
		timer.tv_nsec = QUARTER_SECOND_AS_NSEC;
		nanosleep(&timer, NULL);
		file_exists = access(path, F_OK) == 0;
	}
	if (file_exists) {
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
 * Static Function: is_pwm_exported
 *--------------------------------------*/
static bool is_pwm_exported(uint8_t chip, uint8_t channel) {
	char pwm_direction_path[MAX_FILE_PATH_LENGTH + 1U] = { 0 };
	(void)snprintf(pwm_direction_path, MAX_FILE_PATH_LENGTH, "%s/pwmchip%u/pwm-%u:%u", PWM_CHIP_PATH, chip, chip, channel);
	return (access(pwm_direction_path, F_OK) == 0);
}

/*--------------------------------------
 * Static Function: write_int__to_file
 *--------------------------------------*/
static void write_int_to_file(const char *path, uint32_t value) {
	char value_as_string[SYSFS_PWM_MAX_BUFFER_SIZE + 1U] = { 0 };
	(void)snprintf(value_as_string, SYSFS_PWM_MAX_BUFFER_SIZE, "%" PRIu32, value);
	write_to_file(path, value_as_string);
}

/*--------------------------------------
 * Function: map_ehrpwm_to_sysfs
 *--------------------------------------*/
void map_ehrpwm_to_sysfs(uint8_t ehrpwm_chip, char ehrpwm_channel, uint8_t *chip_out, uint8_t *channel_out) {
	if (ehrpwm_chip == 1U) {
		*chip_out = (uint8_t)4; /* EHRPWM1 maps to pwmchip4 */
	} else if (ehrpwm_chip == 2U) {
		*chip_out = (uint8_t)7; /* EHRPWM2 maps to pwmchip7 */
	} else {
		LOG_AND_EXIT("Invalid EHRPWM chip value: %u", ehrpwm_chip);
	}

	if ((ehrpwm_channel == 'a') || (ehrpwm_channel == 'A')) {
		*channel_out = (uint8_t)0;
	} else if ((ehrpwm_channel == 'b') || (ehrpwm_channel == 'B')) {
		*channel_out = (uint8_t)1;
	} else {
		LOG_AND_EXIT("Invalid EHRPWM channel value: %c", ehrpwm_channel);
	}
}

/*--------------------------------------
 * Function: configure_ehrpwm_pinmux
 *--------------------------------------*/
void configure_ehrpwm_pinmux(uint8_t ehrpwm_chip, char ehrpwm_channel) {
	const char *pin_name = NULL;
	if (ehrpwm_chip == 1U) {
		if ((ehrpwm_channel == 'a') || (ehrpwm_channel == 'A')) {
			pin_name = "P9_14"; /* EHRPWM1A */
		} else if ((ehrpwm_channel == 'b') || (ehrpwm_channel == 'B')) {
			pin_name = "P9_16"; /* EHRPWM1B */
		} else {
			/* MISRA requires else */
		}
	} else if (ehrpwm_chip == 2U) {
		if ((ehrpwm_channel == 'a') || (ehrpwm_channel == 'A')) {
			pin_name = "P8_19"; /* EHRPWM2A */
		} else if ((ehrpwm_channel == 'b') || (ehrpwm_channel == 'B')) {
			pin_name = "P8_13"; /* EHRPWM2B */
		} else {
			/* MISRA requires else */
		}
	} else {
		/* MISRA requires else */
	}

	if (pin_name == NULL) {
		LOG_AND_EXIT("Could not resolve BBB pin for EHRPWM chip %u channel %c", ehrpwm_chip, ehrpwm_channel);
		return;
	}

	char path[MAX_FILENAME_LENGTH + 1U] = { 0 };
	(void)snprintf(path, MAX_FILENAME_LENGTH, "/sys/devices/platform/ocp/ocp:%s_pinmux/state", pin_name);
	FILE *pinmux_file = fopen(path, "w");
	if (pinmux_file == NULL) {
		LOG_AND_EXIT("Failed to open pinmux state file: %s", path);
		return;
	}
	if (fprintf(pinmux_file, "pwm") < 0) {
		(void)fclose(pinmux_file);
		LOG_AND_EXIT("Failed to write pwm to pinmux state file: %s", path);
		return;
	}
	(void)fclose(pinmux_file);
}

/*--------------------------------------
 * Function: init_pwm_channel
 *--------------------------------------*/
void init_pwm_channel(uint8_t chip, uint8_t channel) {
	if (!is_pwm_exported(chip, channel)) {
		export_pwm_channel(chip, channel);

		char pwm_channel_path[MAX_FILE_PATH_LENGTH + 1U] = { 0 };
		(void)snprintf(pwm_channel_path, MAX_FILE_PATH_LENGTH, "%s/pwmchip%u/pwm-%u:%u", PWM_CHIP_PATH, chip, chip, channel);

		if (!wait_for_file(pwm_channel_path)) {
			LOG_AND_EXIT("Failed to export pwm %s", pwm_channel_path);
		}
	}
}

/*--------------------------------------
 * Function: export_gpio_channel
 *--------------------------------------*/
void export_pwm_channel(uint8_t chip, uint8_t channel) {
	char export_path[SYSFS_PWM_MAX_BUFFER_SIZE + 1U] = { 0 };
	(void)snprintf(export_path, SYSFS_PWM_MAX_BUFFER_SIZE, "%s/pwmchip%u/export", PWM_CHIP_PATH, chip);
	write_int_to_file(export_path, channel);
}

/*--------------------------------------
 * Function: unexport_gpio_channel
 *--------------------------------------*/
void unexport_pwm_channel(uint8_t chip, uint8_t channel) {
	char unexport_path[SYSFS_PWM_MAX_BUFFER_SIZE + 1U] = { 0 };
	(void)snprintf(unexport_path, SYSFS_PWM_MAX_BUFFER_SIZE, "%s/pwmchip%u/unexport", PWM_CHIP_PATH, chip);
	/* We want to ignore any errors received here, this unexport is best effort just so exporting always succeeds */
	FILE *fp = fopen(unexport_path, "w");
	if (fp != NULL) {
		(void)fprintf(fp, "%u", channel);
		(void)fclose(fp);
	}
}

/*--------------------------------------
 * Function: set_pwm_period
 *--------------------------------------*/
void set_pwm_period(uint8_t chip, uint8_t channel, uint32_t period_ns) {
	char path[MAX_FILE_PATH_LENGTH + 1U] = { 0 };
	(void)snprintf(path, MAX_FILE_PATH_LENGTH, "%s/pwmchip%u/pwm-%u:%u/period", PWM_CHIP_PATH, chip, chip, channel);
	write_int_to_file(path, period_ns);
}

/*--------------------------------------
 * Function: set_pwm_duty_cycle
 *--------------------------------------*/
int32_t set_pwm_duty_cycle(uint8_t chip, uint8_t channel, uint32_t duty_ns) {
	char path[MAX_FILE_PATH_LENGTH + 1U] = { 0 };
	(void)snprintf(path, MAX_FILE_PATH_LENGTH, "%s/pwmchip%u/pwm-%u:%u/duty_cycle", PWM_CHIP_PATH, chip, chip, channel);
	FILE *file = fopen(path, "w");
	if (file == NULL) {
		return STATUS_FAIL;
	}
	if (fprintf(file, "%" PRIu32, duty_ns) < 0) {
		(void)fclose(file);
		return STATUS_FAIL;
	}
	(void)fclose(file);
	return STATUS_SUCCESS;
}

/*--------------------------------------
 * Function: enable_pwm
 *--------------------------------------*/
void enable_pwm(uint8_t chip, uint8_t channel, bool enable) {
	char path[MAX_FILE_PATH_LENGTH + 1U] = { 0 };
	(void)snprintf(path, MAX_FILE_PATH_LENGTH, "%s/pwmchip%u/pwm-%u:%u/enable", PWM_CHIP_PATH, chip, chip, channel);
	write_int_to_file(path, enable ? 1U : 0U);
}
