#include "led.h"
#include <time.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "gpio_control.h"
#include "heartbeat.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

static global_values_t *shared_info = NULL;

static void handle_system_led_state(void) {
	pthread_mutex_lock(&shared_info->mutex);
	bool is_system_healthy = !((shared_info->current_state == STATE_FAIL) || (shared_info->current_state == STATE_FAIL_SAFE));
	pthread_mutex_unlock(&shared_info->mutex);

	if (is_system_healthy) {
		gpio_clear(shared_info->config.gpio_layout.system_fail_led);
		gpio_set(shared_info->config.gpio_layout.system_ok_led, GPIO_HIGH);
	} else {
		gpio_clear(shared_info->config.gpio_layout.system_ok_led);
		gpio_set(shared_info->config.gpio_layout.system_fail_led, GPIO_HIGH);
	}
}

/* This routine does practically nothing each iteration so we can run it very frequently */
void *led_thread_entry(void *arg) {
	LOG(LED, "Starting LED thread");
	shared_info = (global_values_t *)arg;
	/* 100 ms sleep timer */
	static const struct timespec thread_sleep = { .tv_sec = 0L, .tv_nsec = 100 * NSEC_PER_MSEC };
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		handle_system_led_state();
		increment_heartbeat(shared_info, LED);
		nanosleep(&thread_sleep, NULL);
	}
	LOG(LED, "Shutting down LED thread");
	return NULL;
}
