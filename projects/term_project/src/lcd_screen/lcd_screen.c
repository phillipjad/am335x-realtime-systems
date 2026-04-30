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

/*
 * Fixed layout (16 chars × 2 rows):
 *   Row 0: "CT    TT    XXXX"  — labels + 4-char state at col 12
 *   Row 1: " YY.Y  ZZ.Z     "  — %5.1f CT at col 0, %5.1f TT at col 6
 *
 * Column map:
 *   0-4   CT value  (row 1) / "CT   " label (row 0)
 *   5     separator space
 *   6-10  TT value  (row 1) / "TT   " label (row 0)
 *   11    separator space
 *   12-15 state abbreviation (row 0 only)
 */
#define LCD_COL_CT_VALUE (0U)
#define LCD_COL_TT_VALUE (6U)
#define LCD_COL_STATE (12U)
#define LCD_TEMP_FMT_LEN (5U) /* "%5.1f" always produces 5 chars for range -99.9..999.9 */
#define LCD_STATE_LEN (4U)
#define LCD_TEMP_THRESHOLD (0.1)

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
	timer.tv_nsec = LCD_ENABLE_PULSE_NS;

	lcd_write_data(fd, data | LCD_ENABLE_OFFSET);
	nanosleep(&timer, NULL);
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

static const char *state_to_str(state_e state) {
	switch (state) {
	case STATE_IDLE: return "IDLE";
	case STATE_RUNNING: return "RUN ";
	case STATE_FAIL_SAFE: return "SAFE";
	case STATE_FAIL: return "FAIL";
	default: return "????";
	}
}

static void lcd_write_temp(int32_t fd, uint8_t row, uint8_t col, float64_t temp) {
	char buf[LCD_TEMP_FMT_LEN + 1U] = { 0 };
	(void)snprintf(buf, sizeof(buf), "%5.1f", temp);
	lcd_set_cursor(fd, row, col);
	lcd_print(fd, buf);
}

void *lcd_screen_thread_entry(void *arg) {
	LOG(LCD_SCREEN, "Starting LCD screen thread");
	struct timespec timer = { 0 };
	struct timespec init_timer = { 0 };
	timer.tv_nsec = QUARTER_SECOND_AS_NSEC;
	init_timer.tv_nsec = HALF_SECOND_AS_NSEC;
	shared_info = (global_values_t *)arg;

	int32_t fd = shared_info->config.pin_layout.lcd_fd;

	/* Wake up display */
	for (int32_t ii = 0; ii < 3; ++ii) {
		lcd_pulse(fd, (LCD_DATA_PINS & 0xF0) | LCD_COMMAND | LCD_BACKLIGHT_OFFSET);
		nanosleep(&init_timer, NULL);
	}
	lcd_pulse(fd, (LCD_FOUR_BIT_MODE & 0xF0) | LCD_COMMAND | LCD_BACKLIGHT_OFFSET);
	lcd_clear(fd);
	lcd_send_byte(fd, 0x06, LCD_COMMAND); /* auto-increment cursor */
	lcd_send_byte(fd, 0x0C, LCD_COMMAND); /* display on */

	/* Read initial snapshot */
	pthread_mutex_lock(&shared_info->mutex);
	float64_t prev_current_temp = shared_info->current_temp;
	float64_t prev_target_temp = shared_info->target_temp;
	state_e prev_state = shared_info->current_state;
	pthread_mutex_unlock(&shared_info->mutex);

	/* Write static row 0 layout and initial values — never cleared again */
	lcd_clear(fd);
	lcd_set_cursor(fd, 0, 0);
	lcd_print(fd, "CT    TT    ");
	lcd_print(fd, state_to_str(prev_state));
	lcd_write_temp(fd, 1, LCD_COL_CT_VALUE, prev_current_temp);
	lcd_set_cursor(fd, 1, 5);
	lcd_print(fd, " ");
	lcd_write_temp(fd, 1, LCD_COL_TT_VALUE, prev_target_temp);

	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		pthread_mutex_lock(&shared_info->mutex);
		float64_t current_temp = shared_info->current_temp;
		float64_t target_temp = shared_info->target_temp;
		state_e state_snapshot = shared_info->current_state;
		pthread_mutex_unlock(&shared_info->mutex);

		/* Update state abbreviation if changed */
		if (state_snapshot != prev_state) {
			lcd_set_cursor(fd, 0, LCD_COL_STATE);
			lcd_print(fd, state_to_str(state_snapshot));
			prev_state = state_snapshot;
		}

		/* Update CT field if changed beyond noise threshold */
		float64_t ct_diff = current_temp - prev_current_temp;
		if (ct_diff < 0.0) {
			ct_diff = -ct_diff;
		}
		if (ct_diff > LCD_TEMP_THRESHOLD) {
			lcd_write_temp(fd, 1, LCD_COL_CT_VALUE, current_temp);
			prev_current_temp = current_temp;
		}

		/* Update TT field if changed beyond noise threshold */
		float64_t tt_diff = target_temp - prev_target_temp;
		if (tt_diff < 0.0) {
			tt_diff = -tt_diff;
		}
		if (tt_diff > LCD_TEMP_THRESHOLD) {
			lcd_write_temp(fd, 1, LCD_COL_TT_VALUE, target_temp);
			prev_target_temp = target_temp;
		}

		increment_heartbeat(shared_info, LCD_SCREEN);
		(void)nanosleep(&timer, NULL);
	}

	LOG(LCD_SCREEN, "Shutting down LCD screen thread");
	lcd_clear(shared_info->config.pin_layout.lcd_fd);
	return NULL;
}
