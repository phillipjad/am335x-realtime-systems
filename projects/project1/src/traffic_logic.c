#include "traffic_logic.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "led_control.h"
#include "logger.h"
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

void run_traffic_signal(uint16_t green_light_time, gpio_layout_t light_pins) {
	// Use char to say which set of lights are on: ns = 0, ew = 1
	static bool isNSGroup = true;
	// Use to setup all lights to red
	static bool isStart = true;

	// Light pins for rotation
	uint8_t greenLightPin = 0, yellowLightPin = 0, redLightPin = 0;

	// Setup lights
	if (isStart) {
		LOG("Starting Traffic Lights:");
		light_on(RED, light_pins.red_light_ns);
		light_on(RED, light_pins.red_light_ew);
		isStart = false;
	}

	// Setup groups for lights
	if (isNSGroup) {
		// Group subheader
		LOG("\n\tNS GROUP LIGHTS:");
		// NS Group Pins
		greenLightPin = light_pins.green_light_ns;
		yellowLightPin = light_pins.yellow_light_ns;
		redLightPin = light_pins.red_light_ns;
	} else {
		// Group subheader
		LOG("\n\tEW GROUP LIGHTS:");
		// EW Group Pins
		greenLightPin = light_pins.green_light_ew;
		yellowLightPin = light_pins.yellow_light_ew;
		redLightPin = light_pins.red_light_ew;
	}

	// Timer varirables
	struct timespec timer;
	// Yellow + Red 2 seconds before Green
	timer.tv_sec = 2;
	timer.tv_nsec = 0;
	// Yellow light on
	light_on(YELLOW, yellowLightPin);
	LOG("\tgreen in 2 seconds");
	nanosleep(&timer, NULL);

	// Set Green Light Timer
	timer.tv_sec = green_light_time;

	// Turn off red and yellow light
	light_off(RED, redLightPin);
	light_off(YELLOW, yellowLightPin);

	// GREEN LIGHT
	light_on(GREEN, greenLightPin);
	LOG("\tsolid for %d\n", green_light_time);
	// Sleep for stdio input green light time
	nanosleep(&timer, NULL);

	// Start flashing green light since its almost over
	// flash: off -> on -> ...
	LOG("\tFlashing %d times\n", LIGHT_FLASH_TIME);
	light_flash(GREEN, greenLightPin, LIGHT_FLASH_TIME);
	light_off(GREEN, greenLightPin);

	// YELLOW LIGHT
	timer.tv_sec = 5;
	light_on(YELLOW, yellowLightPin);
	LOG("\tsolid for 5 seconds");
	// Sleep for 5 seconds
	nanosleep(&timer, NULL);
	light_off(YELLOW, yellowLightPin);

	// RED LIGHT
	light_on(RED, redLightPin);

	// Flip Light Groups
	isNSGroup = !isNSGroup;
}
