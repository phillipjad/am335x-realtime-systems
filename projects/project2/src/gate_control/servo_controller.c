#include "servo_controller.h"

#include <pthread.h>
#ifdef NDEBUG
#include <time.h>
#endif

#ifdef NDEBUG /* We only need PWM control in release */
#include "pwm_io_logic.h"
#endif /* NDEBUG */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

/* Servo Angles */
#define SERVO_RIGHT (2000000U)
#define SERVO_CENTER (1500000U)
#define SERVO_LEFT (1000000U)

/* Servo Constants */
#define SERVO_PWM_NS (20000000U)
#define GATE_RAISE (SERVO_RIGHT)
#define GATE_LOWER (SERVO_LEFT)

#ifdef NDEBUG
static uint8_t extracted_chip_value = 0;
static uint8_t extracted_channel_value = 0;
#endif

#ifdef NDEBUG
static void servo_init_hw(uint8_t servo_chip, char servo_channel) {
	// Store mapped values
	// Store chip value
	if (servo_chip == 1) {
		extracted_chip_value = (uint8_t)4; /* EHRPWM1 maps to pwmchip4 */
	} else if (servo_chip == 2) {
		extracted_chip_value = (uint8_t)7; /* EHRPWM2 maps to pwmchip7 */
	} else {
		LOG_AND_EXIT("ERROR: Invalid EHRPWM chip value given in hardware init.");
		return;
	}
	// Get mapped channel uint8_t value
	if ((servo_channel == 'a') || (servo_channel == 'A')) {
		extracted_channel_value = (uint8_t)0;
	} else if ((servo_channel == 'b') || (servo_channel == 'B')) {
		extracted_channel_value = (uint8_t)1;
	} else {
		LOG_AND_EXIT("ERROR: Invalid EHRPWM channel value given in hardware init.");
		return;
	}
	// Export the PWM channel
	init_pwm_channel(extracted_chip_value, extracted_channel_value);
	// Set period
	set_pwm_period(extracted_chip_value, extracted_channel_value, SERVO_PWM_NS);
	// Set duty cycle
	set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, GATE_RAISE);
	// Enable output
	enable_pwm(extracted_chip_value, extracted_channel_value, true);
}
#else
static void servo_init_sw() {
	LOG("Servo initialized");
}
#endif /* NDEBUG */

/*--------------------------------------
 * Function: servo_init
 *--------------------------------------*/
void servo_init(uint8_t servo_chip, char servo_channel) {
#ifdef NDEBUG
	servo_init_hw(servo_chip, servo_channel);
#else
	(void)servo_chip;
	(void)servo_channel;
	servo_init_sw();
#endif /* NDEBUG */
}

/*--------------------------------------
 * Function: servo_raise
 *--------------------------------------*/
void servo_raise(void) {
#ifdef NDEBUG
	// Set duty cycle
	set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, GATE_RAISE);
#else
	LOG("Raising gate");
#endif /* NDEBUG */
}

/*--------------------------------------
 * Function: servo_lower
 *--------------------------------------*/
void servo_lower(void) {
#ifdef NDEBUG
	// Set duty cycle
	set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, GATE_LOWER);
#else
	LOG("Lowering gate");
#endif /* NDEBUG */
}

/*--------------------------------------
 * Function: servo_shutdown
 *--------------------------------------*/
void servo_shutdown(void) {
	LOG("Shutting down gate");
#ifdef NDEBUG
	// Set duty cycle
	set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, GATE_RAISE);
	struct timespec timer = { 0 };
	timer.tv_sec = 0;
	timer.tv_nsec = SEC_TO_NSEC / 2;
	nanosleep(&timer, NULL);
	// Disable output
	enable_pwm(extracted_chip_value, extracted_channel_value, false);
	// Unexport
	unexport_pwm_channel(extracted_chip_value, extracted_channel_value);
#endif /* NDEBUG */
}
