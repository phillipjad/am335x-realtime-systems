#include "led_control.h"

#include <stdint.h>
#include <string.h>
#include <time.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

// Get user_config
extern configuration_items_t user_config;

/*--------------------------------------
 * Function: get_pin_for_light
 *--------------------------------------*/
uint8_t get_pin_for_light(const char *color, const char *direction) {
	if (strcmp(direction, NORTH_SOUTH) == 0) {
		if (strcmp(color, GREEN) == 0) {
			return user_config.gpio_layout.green_light_ns;
		} else if (strcmp(color, YELLOW) == 0) {
			return user_config.gpio_layout.yellow_light_ns;
		} else {
			return user_config.gpio_layout.red_light_ns;
		}
	} else {
		if (strcmp(color, GREEN) == 0) {
			return user_config.gpio_layout.green_light_ew;
		} else if (strcmp(color, YELLOW) == 0) {
			return user_config.gpio_layout.yellow_light_ew;
		} else {
			return user_config.gpio_layout.red_light_ew;
		}
	}
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
	// TODO: Add hardware control
	(void)light_pin;
	return;
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
	// TODO: Add hardware control
	(void)light_pin;
	return;
}

/*--------------------------------------
 * Function: light_on
 *--------------------------------------*/
void light_on(const char *color, const char *direction) {
	light_on_sw(color, direction);
// If in release, use hardware pin to turn light on
#ifdef NDEBUG
	light_on_hw(color, direction);
#else
	// Called so program can compile
	(void)direction;
#endif
}

/*--------------------------------------
 * Function: light_off
 *--------------------------------------*/
void light_off(const char *color, const char *direction) {
	light_off_sw(color, direction);
// If in release, use hardware pin to turn light off
#ifdef NDEBUG
	light_off_hw(color, direction);
#else
	// Called so program can compile
	(void)direction;
#endif
}

/*--------------------------------------
 * Function: light_solid
 *--------------------------------------*/
void light_solid(const char *color, const char *direction, float64_t time) {
	// Timer varirables
	struct timespec timer;
	// Extract seconds
	timer.tv_sec = time;
	// Extract remainder in nanoseconds and multiply by 1B to get seconds
	timer.tv_nsec = (long)((time - timer.tv_sec) * 1000000000L);
	light_on(color, direction);
	nanosleep(&timer, NULL);
	light_off(color, direction);
}

/*--------------------------------------
 * Function: light_flash
 *--------------------------------------*/
void light_flash(const char *color, const char *direction, int flash_count) {
	// Timer varirables
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
