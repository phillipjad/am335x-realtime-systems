#include "light_colors.h"
#include "logger.h"
#include <time.h>

void light_on_stdio(light_color color) {
	LOG("%s is on!", light_color_strings[color]);
}

void light_on_led(light_color color, gpio_pin_layout_t light_pins) {
	// TODO: Add hardware control
	return;
}

void light_off_stdio(light_color color) {
	LOG("%s is off!", light_color_strings[color]);
}

void light_off_led(light_color color, gpio_pin_layout_t light_pins) {
	// TODO: Add hardware control
	return;
}

void light_on(light_color color, gpio_pin_layout_t light_pins) {
	light_on_stdio(color);
// If in release, use hardware pin to turn light on
#ifdef NDEBUG
	light_on_led(color, light_pins);
#endif
}

void light_off(light_color color, gpio_pin_layout_t light_pins) {
	light_off_stdio(color);
// If in release, use hardware pin to turn light off
#ifdef NDEBUG
	light_off_led(color, light_pins);
#endif
}

void light_solid(light_color color, gpio_pin_layout_t light_pins, float_64 time) {
	// Timer varirables
	struct timespec timer;
	timer.tv_sec = 1;
	timer.tv_nsec = 0;
	light_on(color, light_pins);
	nanosleep(&timer, NULL);
	light_off(color, light_pins);
}

void light_flash(light_color color, gpio_pin_layout_t light_pins, int flash_count) {
	// Timer varirables
	struct timespec timer;
	timer.tv_sec = 1;
	timer.tv_nsec = 0;
	// Flash flash_count times at 1 Hz
	for (int i = 0; i < flash_count; i++) {
		light_off(color, light_pins);
		// Sleep for 1 Hz
		nanosleep(&timer, NULL);
		light_on(color, light_pins);
		// Sleep for 1 Hz
		nanosleep(&timer, NULL);
	}
}
