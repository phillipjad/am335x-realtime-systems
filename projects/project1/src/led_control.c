#include "led_control.h"

#include <time.h>

/* Local project includes after system libraries */
#include "logger.h"


/*--------------------------------------
 * Function: light_on_sw
 *--------------------------------------*/
void light_on_sw(const char *color) {
	LOG("%s light is on!", color);
}

/*--------------------------------------
 * Function: light_on_hw
 *--------------------------------------*/
void light_on_hw(const char *color, uint8_t light_pin) {
	LOG("%s light (pin: %d) is on!", color, light_pin);
	// TODO: Add hardware control
	return;
}

/*--------------------------------------
 * Function: light_off_sw
 *--------------------------------------*/
void light_off_sw(const char *color) {
	LOG("%s light is off.", color);
}

/*--------------------------------------
 * Function: light_off_hw
 *--------------------------------------*/
void light_off_hw(const char *color, uint8_t light_pin) {
	LOG("%s light (pin: %d) is off.", color, light_pin);
	// TODO: Add hardware control
	return;
}

/*--------------------------------------
 * Function: light_on
 *--------------------------------------*/
void light_on(const char *color, uint8_t light_pin) {
	light_on_sw(color);
// If in release, use hardware pin to turn light on
#ifdef NDEBUG
	light_on_hw(color, light_pin);
#else
	// Called so program can compile
	(void)light_pin;
#endif
}

/*--------------------------------------
 * Function: light_off
 *--------------------------------------*/
void light_off(const char *color, uint8_t light_pin) {
	light_off_sw(color);
// If in release, use hardware pin to turn light off
#ifdef NDEBUG
	light_off_hw(color, light_pin);
#else
	// Called so program can compile
	(void)light_pin;
#endif
}

/*--------------------------------------
 * Function: light_solid
 *--------------------------------------*/
void light_solid(const char *color, uint8_t light_pin, float64_t time) {
	// Timer varirables
	struct timespec timer;
	timer.tv_sec = time;
	timer.tv_nsec = 0;
	light_on(color, light_pin);
	nanosleep(&timer, NULL);
	light_off(color, light_pin);
}

/*--------------------------------------
 * Function: light_flash
 *--------------------------------------*/
void light_flash(const char *color, uint8_t light_pin, int flash_count) {
	// Timer varirables
	struct timespec timer;
	timer.tv_sec = 1;
	timer.tv_nsec = 0;
	// Flash flash_count times at 1 Hz
	for (int i = 0; i < flash_count; i++) {
		light_off(color, light_pin);
		// Sleep for 1 Hz
		nanosleep(&timer, NULL);
		light_on(color, light_pin);
		// Sleep for 1 Hz
		nanosleep(&timer, NULL);
	}
}
