#include "servo_controller.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <time.h>

#include "gpio_control.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

static global_values_t *shared_data = { 0 };
static uint8_t servo_pin = { 0 };

void sensor_init(global_values_t *shared_info, uint8_t pin) {
	shared_data = shared_info;
	servo_pin = pin;

	// Set pin direction
	gpio_set_direction(pin, GPIO_OUT);
	pthread_create(&pwm, const pthread_attr_t *restrict _Nullable, void *_Nullable (*_Nonnull)(void *_Nullable), void *restrict _Nullable)
}
