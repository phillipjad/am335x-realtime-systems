#include "lcd_screen.h"
#include <stdio.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "heartbeat.h"
#include "logger.h"
#include "project_types.h"
#include "project_constants.h"

#ifdef NDEBUG /* Use LCD packages for release mode */
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#endif /* NDEBUG */

/** Time represented in nanoseconds */
#define QUARTER_SECOND_AS_NSEC (250000000L)
#define HALF_SECOND_AS_NSEC (5000000L)

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

#ifdef NDEBUG
static int lcd_init_hw(const char *i2c_path, uint8_t lcd_addr) {
	int fd = open(i2c_path, O_RDWR);
	if (fd < 0) {
		LOG_AND_EXIT("LCD - Failed to open I2C bus: %s", i2c_path);
	}

	if (ioctl(fd, I2C_SLAVE, lcd_addr)) {
		close(fd);
		LOG_AND_EXIT("LCD - Failed to talk with slave at 0x%02X", lcd_addr);
	}
	return fd;
}
#else
static void lcd_init_sw() {
	LOG("LCD initialized");
}
#endif /* NDEBUG */

int lcd_init(const char *i2c_path, uint8_t lcd_addr) {
#ifdef NDEBUG
	return lcd_init_hw(i2c_path, lcd_addr);
#else
	(void)i2c_path;
	(void)lcd_addr;
	lcd_init_sw();
	return 0;
#endif /* NDEBUG */
}

static void lcd_write_data(int fd, uint8_t data) {
#ifdef NDEBUG
	int result = write(fd, &data, 1);
	if (result < 0) {
		LOG("LCD - Failed to write: %u to fd: %d", data, fd);
	}
#else
	LOG("LCD - Writing: %u", data);
#endif /* NDEBUG */
}

static void lcd_pulse(int fd, uint8_t data) {
#ifdef NDEBUG
	struct timespec timer = { 0 };
	timer.tv_nsec = HALF_SECOND_AS_NSEC;

	// Write with high
	lcd_write_data(fd, data | LCD_ENABLE_OFFSET);
	nanosleep(&timer, NULL);
	// Confrim write with low
	lcd_write_data(fd, data & ~LCD_ENABLE_OFFSET);
	nanosleep(&timer, NULL);
#else
	LOG("LCD - Pulsing data: %u", data);
#endif /* NDEBUG */
}

static void lcd_send_byte(int fd, uint8_t data, uint8_t register_select_mode) {
#ifdef NDEBUG
	lcd_pulse(fd, (data & 0xF0) | register_select_mode | LCD_BACKLIGHT_OFFSET);
	lcd_pulse(fd, ((data << 4) & 0xF0) | register_select_mode | LCD_BACKLIGHT_OFFSET);
#else
	LOG("LCD - Send byte: %u", data);
#endif /* NDEBUG */
}

void lcd_clear(int fd) {
#ifdef NDEBUG
	lcd_send_byte(fd, LCD_CLEAR, LCD_COMMAND);
#else
	LOG("LCD - Clearing screen");
#endif /* NDEBUG */
}

// Only using two rows in screen
void lcd_set_cursor(int fd, uint8_t row, uint8_t col) {
#ifdef NDEBUG
	if (row == 0) {
		lcd_send_byte(fd, LCD_SET_CURSOR | (col + LCD_ROW_ZERO), LCD_COMMAND);
	} else {
		lcd_send_byte(fd, LCD_SET_CURSOR | (col + LCD_ROW_ONE), LCD_COMMAND);
	}
#else
	if (row == 0) {
		LOG("Cursor set to row 0, column %u", col);
	} else {
		LOG("Cursor set to row 1, column %u", col);
	}
#endif /* NDEBUG */
}

void lcd_print(int fd, const char *str) {
	while (*str != '\0') {
		lcd_send_byte(fd, *str++, LCD_DATA);
	}
}

void lcd_print_float(int fd, float64_t val) {
	char buffer[16] = { 0 };
	snprintf(buffer, sizeof(buffer), "%.2f", val);
	lcd_print(fd, buffer);
}

static float64_t get_current_time() {
	struct timespec t = { 0 };
	(void)clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	return t.tv_sec + ((float64_t)t.tv_nsec / SEC_TO_NSEC);
}

static global_values_t *shared_info = NULL;

void *lcd_screen_thread_entry(void *arg) {
	LOG("Starting LCD screen thread");
	struct timespec timer = { 0 };
	struct timespec init_timer = { 0 };
	timer.tv_nsec = QUARTER_SECOND_AS_NSEC;
	init_timer.tv_nsec = HALF_SECOND_AS_NSEC;
	shared_info = (global_values_t *)arg;

	// Setup internal values
	int fd = shared_info->config.gpio_layout.lcd_fd;
	float64_t latest_target_temp = 0;
	float64_t latest_target_temp_timestamp = 0;

	// Wake up display
	for (int i = 0; i < 3; ++i) {
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

	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		// Lock thread
		pthread_mutex_lock(&shared_info->mutex);
		float64_t current_temp = shared_info->current_temp;
		float64_t target_temp = shared_info->target_temp;
		state_t state_snapshot = shared_info->current_state;
		increment_heartbeat(shared_info, LCD_SCREEN);
		// Unlock thread
		pthread_mutex_unlock(&shared_info->mutex);

		// Check if target temp changed
		if (latest_target_temp != target_temp) {
			latest_target_temp = target_temp;
			latest_target_temp_timestamp = get_current_time();
		}

		if (state_snapshot == STATE_RUNNING) {
			float64_t current_time = get_current_time();

			if ((current_time - latest_target_temp_timestamp) <= 2.00) {
				// Print on LCD - Target Temp: ##
				lcd_clear(fd);
				lcd_set_cursor(fd, 0, 0);
				lcd_print(fd, "Target Temp:");
				lcd_set_cursor(fd, 1, 0);
				lcd_print_float(fd, target_temp);
			} else {
				// Print on LCD - Current temperature: ###
				lcd_clear(fd);
				lcd_set_cursor(fd, 0, 0);
				lcd_print(fd, "Current Temp:");
				lcd_set_cursor(fd, 1, 0);
				lcd_print_float(fd, current_temp);
			}
		} else if (state_snapshot == STATE_FAIL_SAFE) {
			// Print on LCD - ERROR: LCD SCREEN STATE FAIL-SAFE
			lcd_clear(fd);
			lcd_set_cursor(fd, 0, 0);
			lcd_print(fd, "ERROR: FAIL-SAFE STATE");
		} else {
			// Print on LCD - ERROR: LCD SCREEN STATE FAILURE
			lcd_clear(fd);
			lcd_set_cursor(fd, 0, 0);
			lcd_print(fd, "ERROR: FAIL STATE");
		}
		nanosleep(&timer, NULL);
	}
	LOG("Shutting down LCD screen thread");
	return NULL;
}
