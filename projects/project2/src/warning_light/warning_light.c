#include "warning_light.h"

/* Local project includes after system libraries */
#include "gpio_control.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include <pthread.h>
#include <time.h>

static global_values_t *shared_info = NULL;

#ifdef NDEBUG
static void warning_lights_blink_hw(void) {
	uint8_t led1_gpio = shared_info->config.gpio_layout.led_1;
	uint8_t led2_gpio = shared_info->config.gpio_layout.led_2;

	struct timespec blink_period = { 0 };
	blink_period.tv_nsec = WARNING_LIGHT_BLINK_MS * NSEC_PER_MSEC;

	/* LED1 on, LED2 off */
	gpio_set(led1_gpio, true);
	gpio_clear(led2_gpio);
	nanosleep(&blink_period, NULL);

	/* LED1 off, LED2 on */
	gpio_clear(led1_gpio);
	gpio_set(led2_gpio, true);
	nanosleep(&blink_period, NULL);

	/* LED1 off, LED2 off */
	gpio_clear(led2_gpio);
}
#else
static void warning_lights_blink_sw(void) {
	LOG("Blinking warning light 1");
	LOG("Blinking warning light 2");
}
#endif /* NDEBUG */

static void warning_lights_blink(void) {
#ifdef NDEBUG
	warning_lights_blink_hw();
#else
	warning_lights_blink_sw();
#endif
}

/*--------------------------------------
 * Function: warning_light_thread_entry
 *--------------------------------------*/
void *warning_light_thread_entry(void *arg) {
	LOG("Starting warning light thread!");
	shared_info = (global_values_t *)arg;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		state_t current_state = STATE_INVALID;
		pthread_mutex_lock(&shared_info->mutex);
		current_state = shared_info->current_state;
		pthread_mutex_unlock(&shared_info->mutex);
		bool light_should_be_active = (current_state == STATE_ACTIVE) || (current_state == STATE_FAIL_SAFE);

		while (light_should_be_active) {
			/* Blink lights and then loop */
			warning_lights_blink();
			pthread_mutex_lock(&shared_info->mutex);
			current_state = shared_info->current_state;
			pthread_mutex_unlock(&shared_info->mutex);
			light_should_be_active = (current_state == STATE_ACTIVE) || (current_state == STATE_FAIL_SAFE);
		}

		/* This thread must activate within 200ms of a button press so sleep is only 10ms. Each cycle is minimal */
		struct timespec timer = { 0 };
		timer.tv_nsec = WARNING_LIGHT_THREAD_SLEEP_MS * NSEC_PER_MSEC;
		nanosleep(&timer, NULL);
	}

	LOG("Shutting down warning light thread");
	return NULL;
}
