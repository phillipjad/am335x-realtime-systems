#include "warning_light.h"

/* Local project includes after system libraries */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include <pthread.h>

static void warning_lights_blink_hw(void) {
	return;
}

static void warning_lights_blink_sw(void) {
	LOG("Blinking warning light 1");
	LOG("Blinking warning light 2");
}

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
	global_values_t *shared_info = (global_values_t *)arg;
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
