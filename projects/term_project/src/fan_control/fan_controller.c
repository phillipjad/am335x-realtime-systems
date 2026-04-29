#include "fan_controller.h"
#include <time.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "pwm_io_logic.h"

/* 25 kHz — Intel 4-pin fan PWM specification */
#define FAN_PWM_PERIOD_NS (40000U)
#define FAN_DUTY_OFF (0U)

static uint8_t extracted_chip_value = 0;
static uint8_t extracted_channel_value = 0;

/*--------------------------------------
 * Function: fan_controller_init
 *--------------------------------------*/
void fan_controller_init(uint8_t fan_chip, char fan_channel) {
	map_ehrpwm_to_sysfs(fan_chip, fan_channel, &extracted_chip_value, &extracted_channel_value);
	configure_ehrpwm_pinmux(fan_chip, fan_channel);
	init_pwm_channel(extracted_chip_value, extracted_channel_value);
	set_pwm_period(extracted_chip_value, extracted_channel_value, FAN_PWM_PERIOD_NS);
	set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, FAN_DUTY_OFF);
	enable_pwm(extracted_chip_value, extracted_channel_value, true);
}

/*--------------------------------------
 * Function: fan_set_speed_pct
 *--------------------------------------*/
int32_t fan_set_speed_pct(uint8_t percent) {
	uint32_t duty_ns = (uint32_t)(((float64_t)percent / 100.0) * (float64_t)FAN_PWM_PERIOD_NS);
	return set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, duty_ns);
}

/*--------------------------------------
 * Function: fan_shutdown
 *--------------------------------------*/
void fan_shutdown(void) {
	LOG(FAN_CONTROL, "Shutting down fan");
	set_pwm_duty_cycle(extracted_chip_value, extracted_channel_value, FAN_DUTY_OFF);
	static const struct timespec settle = { .tv_sec = 0L, .tv_nsec = 500000000L };
	(void)nanosleep(&settle, NULL);
	enable_pwm(extracted_chip_value, extracted_channel_value, false);
	unexport_pwm_channel(extracted_chip_value, extracted_channel_value);
}
