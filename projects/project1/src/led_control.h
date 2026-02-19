#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "project_constants.h"

/**
 * @brief Software version of LED turns on through LOG
 *
 * @param[in] color Traffic Light color
 */
void light_on_stdio(const char *color);

/**
 * @brief Hardware version of LED turns on through pins
 *
 * @param[in] color Traffic Light color
 * @param[in] light_pins Pin number for LEDs corresponding to Traffic Light
 */
void light_on_led(const char *color, gpio_pin_layout_t light_pins);

/**
 * @brief Software version of LED turns off through LOG
 *
 * @param[in] color Traffic Light color
 */
void light_off_stdio(const char *color);

/**
 * @brief Hardware version of LED turns off through pins
 *
 * @param[in] color Traffic Light color
 * @param[in] light_pins Pin number for LEDs corresponding to Traffic Light
 */
void light_off_led(const char *color, gpio_pin_layout_t light_pins);

/**
 * @brief Overarching function to invoke hardware and software versions of turning LED on
 *
 * @param[in] color Traffic Light color
 * @param[in] light_pins Pin number for LEDs corresponding to Traffic Light
 */
void light_on(const char *color, gpio_pin_layout_t light_pins);

/**
 * @brief Overarching function to invoke hardware and software versions of turning LED off
 *
 * @param[in] color Traffic Light color
 * @param[in] light_pins Pin number for LEDs corresponding to Traffic Light
 */
void light_off(const char *color, gpio_pin_layout_t light_pins);

/**
 * @brief Keep light on certain amount of time
 *
 * @param[in] color Traffic Light color
 * @param[in] light_pins Pin number for LEDs corresponding to Traffic Light
 * @param[in] time Amount to keep light on for
 */
void light_solid(const char *color, gpio_pin_layout_t light_pins, float_64 time);

/**
 * @brief Flash light on certain number of times
 *
 * @param[in] color Traffic Light color
 * @param[in] light_pins Pin number for LEDs corresponding to Traffic Light
 * @param[in] flash_count Number of times to turn light off -> on and viseversa
 */
void light_flash(const char *color, gpio_pin_layout_t light_pins, int flash_count);
#endif /* LED_CONTROL_H */
