#include "warning_light.h"

#include <pthread.h>
#include <time.h>

/* Local project includes after system libraries */
#ifdef NDEBUG /* We only need GPIO control in release */
#include "gpio_control.h"
#endif /* NDEBUG */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

/** Variable to point to shared global struct in this TU */
static global_values_t *shared_info = NULL;
/** Timespec describing how long a warning light blink should be active for */
static const struct timespec blink_period = { .tv_sec = 0, .tv_nsec = WARNING_LIGHT_ACTIVE_DURATION_MS * NSEC_PER_MSEC };

#ifdef NDEBUG
/**
 * @brief Blinks the warning lights using actual GPIO pins
 */
static void warning_lights_blink_hw(void) {
	uint8_t led1_gpio = shared_info->config.gpio_layout.led_1;
	uint8_t led2_gpio = shared_info->config.gpio_layout.led_2;


	/* LED1 on, LED2 off */
	gpio_set(led1_gpio, true);
	gpio_clear(led2_gpio);
	(void)nanosleep(&blink_period, NULL);

	/* LED1 off, LED2 on */
	gpio_clear(led1_gpio);
	gpio_set(led2_gpio, true);
	(void)nanosleep(&blink_period, NULL);

	/* LED1 off, LED2 off */
	gpio_clear(led2_gpio);
}
#else
/**
 * @brief Blinks the warning lights virtually through logs
 */
static void warning_lights_blink_sw(void) {
	LOG("Warning light 1 on");
	(void)nanosleep(&blink_period, NULL);
	LOG("Warning light 1 off");
	LOG("Warning light 2 on");
	(void)nanosleep(&blink_period, NULL);
	LOG("Warning light 2 off");
}
#endif /* NDEBUG */

/**
 * @brief Blinks the warning lights, determining whether software of hardware logic will be used at compile time
 */
static void warning_lights_blink(void) {
#ifdef NDEBUG
	warning_lights_blink_hw();
#else
	warning_lights_blink_sw();
#endif /* NDEBUG */
}

/**
 * @brief This function will continuously blink the warning lights for one second
 */
static void warning_lights_blink_one_second(void) {
	/* Set stop time one second in the future */
	struct timespec stop = { 0 };
	(void)clock_gettime(CLOCK_MONOTONIC_RAW, &stop);
	++stop.tv_sec;

	/* We're going to blink on a loop until we reach stop */
	float64_t sub_result = 0.0;
	do {
		warning_lights_blink();

		/* Get time now */
		struct timespec now = { 0 };
		(void)clock_gettime(CLOCK_MONOTONIC_RAW, &now);

		/* Compare now to when we should be stopping */
		time_t sec_result = (stop.tv_sec - now.tv_sec);
		sub_result = (float64_t)sec_result;
		int64_t nsec_result = (stop.tv_nsec - now.tv_nsec);
		float64_t nsec_result_as_float = nsec_result;
		sub_result += (float64_t)(nsec_result_as_float / (float64_t)SEC_TO_NSEC);
	} while (sub_result > 0.0); /* sub_result being greater than 0.0 means that stop is still in the future */
}

/*--------------------------------------
 * Function: warning_light_thread_entry
 *--------------------------------------*/
void *warning_light_thread_entry(void *arg) {
	LOG("Starting warning light thread!");
	shared_info = (global_values_t *)arg;
	/* This thread must activate within 200ms of a button press so sleep is only 10ms. Each cycle is minimal */
	struct timespec timer = { 0 };
	timer.tv_nsec = WARNING_LIGHT_THREAD_SLEEP_MS * NSEC_PER_MSEC;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		state_t current_state = STATE_INVALID;
		pthread_mutex_lock(&shared_info->mutex);
		current_state = shared_info->current_state;
		pthread_mutex_unlock(&shared_info->mutex);
		bool light_should_be_active = (current_state == STATE_ACTIVE) || (current_state == STATE_FAIL_SAFE);

		/* While in states of interest, we'll make lights blink */
		while (light_should_be_active) {
			/* Blink lights and then loop */
			warning_lights_blink();
			pthread_mutex_lock(&shared_info->mutex);
			current_state = shared_info->current_state;
			pthread_mutex_unlock(&shared_info->mutex);
			light_should_be_active = (current_state == STATE_ACTIVE) || (current_state == STATE_FAIL_SAFE);
		}

		/* After blinking, we should check if we are clearing which requires an additional second of blinking */
		if (current_state == STATE_CLEARING) {
			warning_lights_blink_one_second();
		}

		(void)nanosleep(&timer, NULL);
	}

	LOG("Shutting down warning light thread");
	return NULL;
}
