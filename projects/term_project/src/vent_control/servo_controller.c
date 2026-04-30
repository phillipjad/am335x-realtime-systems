#include "servo_controller.h"

#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "pwm_io_logic.h"

/* Servo Angles */
#define SERVO_CENTER_NS (1500000U) /* 1.5 ms — center position */
#define SERVO_180_NS (2500000U)    /* 2.0 ms — 180 deg position */

/* Servo Constants */
#define SERVO_PERIOD_NS (20000000U)
#define VENT_OPEN (SERVO_180_NS)
#define VENT_CLOSED (SERVO_CENTER_NS)

static uint8_t extracted_chip_value = 0;
static uint8_t extracted_channel_value = 0;

static void servo_init_hw(uint8_t servo_chip, char servo_channel) {
	map_ehrpwm_to_sysfs(servo_chip, servo_channel, &extracted_chip_value, &extracted_channel_value);
	configure_ehrpwm_pinmux(servo_chip, servo_channel);
	init_pwm_channel(extracted_chip_value, extracted_channel_value);
	set_pwm_period(extracted_chip_value, extracted_channel_value, SERVO_PERIOD_NS);
	enable_pwm(extracted_chip_value, extracted_channel_value, true);
	set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, VENT_OPEN);
	struct timespec timer = { 0 };
	timer.tv_nsec = NSEC_PER_SEC / 2L;
	(void)nanosleep(&timer, NULL);
}

/*--------------------------------------
 * Function: servo_init
 *--------------------------------------*/
void servo_init(uint8_t servo_chip, char servo_channel) {
	servo_init_hw(servo_chip, servo_channel);
}

/*--------------------------------------
 * Function: servo_raise
 *--------------------------------------*/
int32_t servo_raise(void) {
	return set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, VENT_OPEN);
}

/*--------------------------------------
 * Function: servo_lower
 *--------------------------------------*/
int32_t servo_lower(void) {
	return set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, VENT_CLOSED);
}

/*--------------------------------------
 * Function: servo_lower
 *--------------------------------------*/
int32_t potentiometer_based_servo(float64_t percent) {
	float64_t duty_value = VENT_CLOSED + ((float64_t)(VENT_OPEN - VENT_CLOSED)) * (percent / 100.0);
	return set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, duty_value);
}

/*--------------------------------------
 * Function: servo_shutdown
 *--------------------------------------*/
void servo_shutdown(void) {
	LOG(NUM_THREADS, "Shutting down gate");
	int32_t result = set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, VENT_CLOSED);
	if (result != STATUS_SUCCESS) {
		LOG(NUM_THREADS, "Failed to set closing duty cycle");
	}
	struct timespec timer = { 0 };
	timer.tv_sec = 1;
	timer.tv_nsec = NSEC_PER_SEC / 2L;
	(void)nanosleep(&timer, NULL);
	enable_pwm(extracted_chip_value, extracted_channel_value, false);
	unexport_pwm_channel(extracted_chip_value, extracted_channel_value);
}
