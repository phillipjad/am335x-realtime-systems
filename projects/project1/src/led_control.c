#include "led_control.h"

#include <stdint.h>
#include <string.h>
#include <time.h>

/* Local project includes after system libraries */
#include "io_logic.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

// Get user_config
extern configuration_items_t user_config;

/*--------------------------------------
 * Function: get_pin_for_light
 *--------------------------------------*/
uint8_t get_pin_for_light(const char *color, const char *direction) {
	uint8_t pin = 0U;
	if (strcmp(direction, NORTH_SOUTH) == 0) {
		if (strcmp(color, GREEN) == 0) {
			pin = user_config.gpio_layout.green_light_ns;
		} else if (strcmp(color, YELLOW) == 0) {
			pin = user_config.gpio_layout.yellow_light_ns;
		} else {
			pin = user_config.gpio_layout.red_light_ns;
		}
	} else {
		if (strcmp(color, GREEN) == 0) {
			pin = user_config.gpio_layout.green_light_ew;
		} else if (strcmp(color, YELLOW) == 0) {
			pin = user_config.gpio_layout.yellow_light_ew;
		} else {
			pin = user_config.gpio_layout.red_light_ew;
		}
	}
	return pin;
}

/*--------------------------------------
 * Function: light_on_sw
 *--------------------------------------*/
void light_on_sw(const char *color, const char *direction) {
	LOG("%s %s lights are on!", direction, color);
}

/*--------------------------------------
 * Function: light_on_hw
 *--------------------------------------*/
void light_on_hw(const char *color, const char *direction) {
	uint8_t light_pin = get_pin_for_light(color, direction);
	signal_gpio(light_pin, PIN_ON);
}

/*--------------------------------------
 * Function: light_off_sw
 *--------------------------------------*/
void light_off_sw(const char *color, const char *direction) {
	LOG("%s %s lights are off.", direction, color);
}

/*--------------------------------------
 * Function: light_off_hw
 *--------------------------------------*/
void light_off_hw(const char *color, const char *direction) {
	uint8_t light_pin = get_pin_for_light(color, direction);
	signal_gpio(light_pin, PIN_OFF);
}

/*--------------------------------------
 * Function: light_on
 *--------------------------------------*/
void light_on(const char *color, const char *direction) {
	light_on_sw(color, direction);
#ifdef NDEBUG /* If in release, use hardware pin to turn light on */
	light_on_hw(color, direction);
#else
#endif
}

/*--------------------------------------
 * Function: light_off
 *--------------------------------------*/
void light_off(const char *color, const char *direction) {
	light_off_sw(color, direction);
#ifdef NDEBUG /* If in release, use hardware pin to turn light on */
	light_off_hw(color, direction);
#endif
}

/*--------------------------------------
 * Function: light_solid
 *--------------------------------------*/
void light_solid(const char *color, const char *direction, float64_t time) {
	// Timer variables
	struct timespec timer;
	// Extract seconds
	timer.tv_sec = (int64_t)time;
	// Extract remainder in nanoseconds and multiply by SEC_TO_NSEC to get seconds
	int64_t sec_tmp = timer.tv_sec;
	float64_t timer_sec_as_double = (float64_t)sec_tmp;
	float64_t nsec_as_float = (time - timer_sec_as_double) * SEC_TO_NSEC;
	int64_t nsec_as_int = (int64_t)nsec_as_float;
	timer.tv_nsec = nsec_as_int;
	light_on(color, direction);
	nanosleep(&timer, NULL);
	light_off(color, direction);
}

/*--------------------------------------
 * Function: light_flash
 *--------------------------------------*/
void light_flash(const char *color, const char *direction, int flash_count) {
	// Timer variables
	struct timespec timer;
	timer.tv_sec = 1;
	timer.tv_nsec = 0;
	// Flash flash_count times at 1 Hz
	for (int i = 0; i < flash_count; ++i) {
		light_off(color, direction);
		// Sleep for 1 Hz
		nanosleep(&timer, NULL);
		light_on(color, direction);
		// Sleep for 1 Hz
		nanosleep(&timer, NULL);
	}
}
