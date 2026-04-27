#include "lcd_screen.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "heartbeat.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

/* LCD packages */
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

/* LCD Screen Macros - https://www.handsontec.com/dataspecs/module/I2C_1602_LCD.pdf*/
#define LCD_REGISTER_OFFSET 0x01
#define LCD_READ_WRTIE_OFFSET 0x02
#define LCD_ENABLE_OFFSET 0x04
#define LCD_BACKLIGHT_OFFSET 0x08

#define LCD_WRITE 0
#define LCD_READ 1
#define LCD_COMMAND 0
#define LCD_DATA 1
#define LCD_BACKLIGHT_OFF 0
#define LCD_BACKLIGHT_ON 1
#define LCD_CLEAR 0x01
#define LCD_ROW_ZERO 0x00
#define LCD_ROW_ONE 0x40
#define LCD_SET_CURSOR 0x80
#define LCD_DATA_PINS 0x30
#define LCD_FOUR_BIT_MODE 0x20
#define LCD_CHAR_SIZE 16

#define LCD_TARGET_SHOW_TIME 2

static global_values_t *shared_info = NULL;

static int32_t lcd_init_hw(const char *i2c_path, uint8_t lcd_addr) {
	int32_t fd = open(i2c_path, O_RDWR);
	if (fd < 0) {
		LOG_AND_EXIT("LCD - Failed to open I2C bus: %s", i2c_path);
	}

	if (ioctl(fd, I2C_SLAVE, lcd_addr)) {
		close(fd);
		LOG_AND_EXIT("LCD - Failed to talk with slave at 0x%02X", lcd_addr);
	}
	return fd;
}

int32_t lcd_init(const char *i2c_path, uint8_t lcd_addr) {
	return lcd_init_hw(i2c_path, lcd_addr);
}

static void lcd_write_data(int32_t fd, uint8_t data) {
	int32_t result = write(fd, &data, 1);
	// Locking because of error checking/settings
	if (result < 0) {
		LOG(LCD_SCREEN, "Failed to write: %u to fd: %d", data, fd);
		pthread_mutex_lock(&shared_info->mutex);
		set_error(&shared_info->thread_errors[LCD_SCREEN], "%s", strerror(errno));
		pthread_mutex_unlock(&shared_info->mutex);
	} else {
		pthread_mutex_lock(&shared_info->mutex);
		clear_error(&shared_info->thread_errors[LCD_SCREEN]);
		pthread_mutex_unlock(&shared_info->mutex);
	}
}

static void lcd_pulse(int32_t fd, uint8_t data) {
	struct timespec timer = { 0 };
	timer.tv_nsec = HALF_SECOND_AS_NSEC;

	// Write with high
	lcd_write_data(fd, data | LCD_ENABLE_OFFSET);
	nanosleep(&timer, NULL);
	// Confirm write with low
	lcd_write_data(fd, data & ~LCD_ENABLE_OFFSET);
	nanosleep(&timer, NULL);
}

static void lcd_send_byte(int32_t fd, uint8_t data, uint8_t register_select_mode) {
	lcd_pulse(fd, (data & 0xF0) | register_select_mode | LCD_BACKLIGHT_OFFSET);
	lcd_pulse(fd, ((data << 4) & 0xF0) | register_select_mode | LCD_BACKLIGHT_OFFSET);
}

void lcd_clear(int32_t fd) {
	lcd_send_byte(fd, LCD_CLEAR, LCD_COMMAND);
}

// Only using two rows in screen
void lcd_set_cursor(int32_t fd, uint8_t row, uint8_t col) {
	if (row == 0) {
		lcd_send_byte(fd, LCD_SET_CURSOR | (col + LCD_ROW_ZERO), LCD_COMMAND);
	} else {
		lcd_send_byte(fd, LCD_SET_CURSOR | (col + LCD_ROW_ONE), LCD_COMMAND);
	}
}

void lcd_print(int32_t fd, const char *str) {
	while (*str != '\0') {
		lcd_send_byte(fd, *str++, LCD_DATA);
	}
}

void lcd_print_float(int32_t fd, float64_t val) {
	char buffer[LCD_CHAR_SIZE] = { 0 };
	snprintf(buffer, sizeof(buffer), "%.2f F", val);
	lcd_print(fd, buffer);
}

static float64_t get_current_time() {
	struct timespec t = { 0 };
	(void)clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	return t.tv_sec + ((float64_t)t.tv_nsec / NSEC_PER_SEC_F);
}

static bool update_lcd(float64_t read_time, float64_t current_time, state_e last_state, state_e current_state) {
	bool update = false;
	if (read_time != 0 && (current_time - read_time) < LCD_TARGET_SHOW_TIME) {
		update = true;
	} else if (last_state != current_state) {
		update = true;
	}
	return update;
}

void *lcd_screen_thread_entry(void *arg) {
	LOG(LCD_SCREEN, "Starting LCD screen thread");
	struct timespec timer = { 0 };
	struct timespec init_timer = { 0 };
	timer.tv_nsec = QUARTER_SECOND_AS_NSEC;
	init_timer.tv_nsec = HALF_SECOND_AS_NSEC;
	shared_info = (global_values_t *)arg;
	bool render_lcd = true;

	// Setup internal values
	int32_t fd = shared_info->config.gpio_layout.lcd_fd;
	float64_t latest_target_temp = 0;
	float64_t latest_target_temp_timestamp = 0;
	state_e last_read_state = STATE_IDLE;

	// Wake up display
	for (int32_t ii = 0; ii < 3; ++ii) {
		lcd_pulse(fd, (LCD_DATA_PINS & 0xF0) | LCD_COMMAND | LCD_BACKLIGHT_OFFSET);
		nanosleep(&init_timer, NULL);
	}
	lcd_pulse(fd, (LCD_FOUR_BIT_MODE & 0xF0) | LCD_COMMAND | LCD_BACKLIGHT_OFFSET);
	// Clear screen
	lcd_clear(fd);
	// Auto increment cursor
	lcd_send_byte(fd, 0x06, LCD_COMMAND);
	// Turn on screen
	lcd_send_byte(fd, 0x0C, LCD_COMMAND);

	// Starting text
	lcd_clear(fd);
	lcd_set_cursor(fd, 0, 0);
	lcd_print(fd, "Starting LCD...");
	render_lcd = false;
	bool was_showing_target = false;

	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		// Lock thread
		pthread_mutex_lock(&shared_info->mutex);
		float64_t current_temp = shared_info->current_temp;
		float64_t target_temp = shared_info->target_temp;
		state_e state_snapshot = shared_info->current_state;
		// Unlock thread
		pthread_mutex_unlock(&shared_info->mutex);

		// Check if target temp changed between certain range to account for noise
		float64_t current_time = get_current_time();
		float64_t temp_difference = latest_target_temp - target_temp;
		if (temp_difference < 0) {
			temp_difference = -temp_difference;
		}
		printf("temp diff: %f\n", temp_difference);
		printf("%f vs %f\n", latest_target_temp, target_temp);

		if (temp_difference > 0.1) {
			latest_target_temp = target_temp;
			latest_target_temp_timestamp = get_current_time();
			render_lcd = update_lcd(latest_target_temp_timestamp, current_time, last_read_state, state_snapshot);
		} else if (last_read_state != state_snapshot) {
			render_lcd = update_lcd(latest_target_temp_timestamp, current_time, last_read_state, state_snapshot);
		}
		bool show_target = (current_time - latest_target_temp_timestamp) <= LCD_TARGET_SHOW_TIME;
		if (was_showing_target && !show_target) {
			render_lcd = true;
		}
		was_showing_target = show_target;

		if (state_snapshot == STATE_RUNNING) {
			// Buffer of temp before updating so we don't spam updates
			// Show target for first 2 sec then current temp
			if (render_lcd && last_read_state != state_snapshot &&
			    ((current_time - latest_target_temp_timestamp) <= LCD_TARGET_SHOW_TIME)) {
				// Print on LCD - Target Temp: ##
				lcd_clear(fd);
				lcd_set_cursor(fd, 0, 0);
				lcd_print(fd, "R: Target Temp");
				lcd_set_cursor(fd, 1, 0);
				lcd_print_float(fd, target_temp);
				LOG(LCD_SCREEN, "Running: Target Temp: %lf", target_temp);
				render_lcd = false;
			} else if (render_lcd && last_read_state != state_snapshot) {
				// Print on LCD - Current temperature: ###
				lcd_clear(fd);
				lcd_set_cursor(fd, 0, 0);
				lcd_print(fd, "R: Current Temp");
				lcd_set_cursor(fd, 1, 0);
				lcd_print_float(fd, current_temp);
				LOG(LCD_SCREEN, "Running: Current Temp: %lf", current_temp);
				render_lcd = false;
			} else {
				/* MISRA requires else */
			}
		} else if (render_lcd && state_snapshot == STATE_FAIL_SAFE) {
			// Print on LCD - ERROR: LCD SCREEN STATE FAIL-SAFE
			if ((current_time - latest_target_temp_timestamp) <= LCD_TARGET_SHOW_TIME) {
				// Print on LCD - Target Temp: ##
				lcd_clear(fd);
				lcd_set_cursor(fd, 0, 0);
				lcd_print(fd, "FS: Target Temp");
				lcd_set_cursor(fd, 1, 0);
				lcd_print_float(fd, target_temp);
				LOG(LCD_SCREEN, "Fail-Safe: Target Temp: %lf", target_temp);
				render_lcd = false;
			} else {
				// Print on LCD - Current temperature: ###
				lcd_clear(fd);
				lcd_set_cursor(fd, 0, 0);
				lcd_print(fd, "FS: Current Temp");
				lcd_set_cursor(fd, 1, 0);
				lcd_print_float(fd, current_temp);
				LOG(LCD_SCREEN, "Fail-Safe: Current Temp: %lf", current_temp);
				render_lcd = false;
			}
		} else if (render_lcd && state_snapshot == STATE_FAIL) {
			// Print on LCD - ERROR: LCD SCREEN STATE FAILURE
			lcd_clear(fd);
			lcd_set_cursor(fd, 0, 0);
			lcd_print(fd, "ERROR:");
			lcd_set_cursor(fd, 1, 0);
			lcd_print(fd, "FAIL STATE");
			LOG(LCD_SCREEN, "FAIL: LCD read Fail state");
			render_lcd = false;
		} else {
			/* MISRA requires else */
		}
		// Update last read state
		last_read_state = state_snapshot;
		increment_heartbeat(shared_info, LCD_SCREEN);
		(void)nanosleep(&timer, NULL);
	}
	LOG(LCD_SCREEN, "Shutting down LCD screen thread");
	return NULL;
}
