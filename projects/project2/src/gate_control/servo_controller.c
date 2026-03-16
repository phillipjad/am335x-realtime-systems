#include "servo_controller.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <time.h>

#include "gpio_control.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

static global_values_t *shared_info = NULL;

void sensor_init(global_values_t *shared_info, uint8_t pin) {
#ifdef NDEBUG
	sensor_init_hw(shared_info, pin);
#else
	sensor_init_sw();
#endif /* NDEBUG */
}

void sensor_init_hw(global_values_t *shared_info, uint8_t pin) {
	shared_data = shared_info;
	servo_pin = pin;

	// Set pin direction
	gpio_set_direction(pin, GPIO_OUT);
}

void sensor_init_sw() {
	LOG("SW: Lowing gate");
}

#ifdef NDEBUG
#else
#endif /* NDEBUG */
