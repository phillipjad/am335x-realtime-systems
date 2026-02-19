#include "traffic_logic.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "led_control.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

void handle_shutdown(void) {
	/* TODO: This will need to actually handle shutdown at some point */
	uint32_t sleep_time = sleep(SHUTDOWN_DELAY_S);
	if (sleep_time == SHUTDOWN_DELAY_S) {
		LOG("Failed to gracefully shutdown");
	}
	LOG("Shutdown sequence successfully completed");
	exit(EXIT_SUCCESS);
}

void run_traffic_signal(uint_16_t green_light_time, gpio_pin_layout_t light_pins) {
	// Use char to say which set of lights are on: ns = 0, ew = 1
	bool isNSGroup = true;

	if (isNSGroup) {
	}
	// Timer varirables
	struct timespec timer;
	timer.tv_sec = green_light_time;
	timer.tv_nsec = 0;

	// GREEN LIGHT
	light_on(GREEN, light_pins);
	// Sleep for stdio input green light time
	nanosleep(&timer, NULL);
	light_flash(GREEN, light_pins, LIGHT_FLASH_TIME);
	light_off(GREEN, light_pins);

	// YELLOW LIGHT
	timer.tv_sec = 5;
	light_on(YELLOW, light_pins);
	// Sleep for 5 seconds
	nanosleep(&timer, NULL);
	light_off(YELLOW, light_pins);

	// RED LIGHT
	timer.tv_sec = 2;
	light_on(RED, light_pins);
	// Sleep for 2 seconds

	// Flip Light Groups
}
