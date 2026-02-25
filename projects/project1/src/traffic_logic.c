#include "traffic_logic.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "led_control.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

/*--------------------------------------
 * Function: handle_shutdown
 *--------------------------------------*/
void handle_shutdown() {
	// Turn off all LEDs
	LOG("Turning off all lights...");
	light_off(GREEN, NORTH_SOUTH);
	light_off(GREEN, EAST_WEST);
	light_off(YELLOW, NORTH_SOUTH);
	light_off(YELLOW, EAST_WEST);
	light_off(RED, NORTH_SOUTH);
	light_off(RED, EAST_WEST);

	LOG("Shutdown sequence successfully completed");
	exit(EXIT_SUCCESS);
}

/*--------------------------------------
 * Function: run_traffic_signal
 *--------------------------------------*/
void run_traffic_signal(uint16_t green_light_time) {
	// Use boolean to say which set of lights are on: ns = true, ew = false
	static bool isNSGroup = true;
	// Use to setup all lights to red
	static bool isStart = true;
	// Set the traffic direction for rotation
	const char *traffic_direction = isNSGroup ? NORTH_SOUTH : EAST_WEST;
	// Float var
	float64_t float_time = 0.0;

	// Setup lights
	if (isStart) {
		DEBUG_LOG("Starting Traffic Lights:");
		light_on(RED, NORTH_SOUTH);
		light_on(RED, EAST_WEST);
		isStart = false;
	}

	// Timer variables
	struct timespec timer;
	// Yellow + Red 2 seconds before Green
	timer.tv_sec = 2;
	timer.tv_nsec = 0;
	// Yellow light on
	light_on(YELLOW, traffic_direction);
	DEBUG_LOG("\tGreen in 2 seconds");
	nanosleep(&timer, NULL);

	// Set Green Light Timer
	// Resolved MISRA23_10.8 and 10.3
	float_time = (float64_t)green_light_time;
	timer.tv_sec = (time_t)float_time;

	// Turn off red and yellow light
	light_off(RED, traffic_direction);
	light_off(YELLOW, traffic_direction);

	// GREEN LIGHT
	light_on(GREEN, traffic_direction);
	DEBUG_LOG("\tSolid for %d seconds", green_light_time);
	// Sleep for stdio input green light time
	nanosleep(&timer, NULL);

	// Start flashing green light since its almost over
	// flash: off -> on -> ...
	DEBUG_LOG("\tFlashing %d times", LIGHT_FLASH_COUNT);
	light_flash(GREEN, traffic_direction, LIGHT_FLASH_COUNT);
	light_off(GREEN, traffic_direction);

	// YELLOW LIGHT
	timer.tv_sec = 5;
	light_on(YELLOW, traffic_direction);
	DEBUG_LOG("\tsolid for 5 seconds");
	// Sleep for 5 seconds
	nanosleep(&timer, NULL);
	light_off(YELLOW, traffic_direction);

	// RED LIGHT
	light_on(RED, traffic_direction);

	// Flip Light Groups
	isNSGroup = !isNSGroup;
}
