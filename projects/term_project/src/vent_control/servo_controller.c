#include "servo_controller.h"

#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "pwm_io_logic.h"

/* Servo Angles */
#define SERVO_RIGHT (2000000U)
#define SERVO_CENTER (1500000U)
#define SERVO_LEFT (1000000U)

/* Servo Constants */
#define SERVO_PWM_NS (20000000U)
#define GATE_RAISE (SERVO_RIGHT)
#define GATE_LOWER (SERVO_CENTER)

static uint8_t extracted_chip_value = 0;
static uint8_t extracted_channel_value = 0;

static void servo_init_hw(uint8_t servo_chip, char servo_channel) {
	map_ehrpwm_to_sysfs(servo_chip, servo_channel, &extracted_chip_value, &extracted_channel_value);
	configure_ehrpwm_pinmux(servo_chip, servo_channel);
	init_pwm_channel(extracted_chip_value, extracted_channel_value);
	set_pwm_period(extracted_chip_value, extracted_channel_value, SERVO_PWM_NS);
	set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, GATE_RAISE);
	enable_pwm(extracted_chip_value, extracted_channel_value, true);
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
	return set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, GATE_RAISE);
}

/*--------------------------------------
 * Function: servo_lower
 *--------------------------------------*/
int32_t servo_lower(void) {
	return set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, GATE_LOWER);
}

/*--------------------------------------
 * Function: servo_lower
 *--------------------------------------*/
int32_t potentiometer_based_servo(float64_t percent) {
	float64_t duty_value = GATE_LOWER + ((float64_t)(GATE_RAISE - GATE_LOWER)) * (percent / 100.0);
	return set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, duty_value);
}

/*--------------------------------------
 * Function: servo_shutdown
 *--------------------------------------*/
void servo_shutdown(void) {
	LOG(VENT_CONTROL, "Shutting down gate");
	set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, GATE_RAISE);
	struct timespec timer = { 0 };
	timer.tv_sec = 0;
	timer.tv_nsec = NSEC_PER_SEC_F / 2;
	nanosleep(&timer, NULL);
	enable_pwm(extracted_chip_value, extracted_channel_value, false);
	unexport_pwm_channel(extracted_chip_value, extracted_channel_value);
}
