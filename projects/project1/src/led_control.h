#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <stdint.h>

/* Local project includes after system libraries */
#include "project_constants.h"
#include "project_types.h"

/**
 * @brief Software version of LED turns on through LOG
 *
 * @param[in] color Traffic Light color
 * @param[in] direction Direction for LEDs corresponding to Traffic Light
 */
void light_on_sw(const char *color, const char *direction);

/**
 * @brief Hardware version of LED turns on through pin
 *
 * @param[in] color Traffic Light color
 * @param[in] direction Direction for LEDs corresponding to Traffic Light
 */
void light_on_hw(const char *color, const char *direction);

/**
 * @brief Software version of LED turns off through LOG
 *
 * @param[in] color Traffic Light color
 * @param[in] direction Direction for LEDs corresponding to Traffic Light
 */
void light_off_sw(const char *color, const char *direction);

/**
 * @brief Hardware version of LED turns off through pin
 *
 * @param[in] color Traffic Light color
 * @param[in] direction Direction for LED corresponding to Traffic Light
 */
void light_off_hw(const char *color, const char *direction);

/**
 * @brief Overarching function to invoke hardware and software versions of turning LED on
 *
 * @param[in] color Traffic Light color
 * @param[in] direction Direction for LED corresponding to Traffic Light
 */
void light_on(const char *color, const char *direction);

/**
 * @brief Overarching function to invoke hardware and software versions of turning LED off
 *
 * @param[in] color Traffic Light color
 * @param[in] direction Direction for LED corresponding to Traffic Light
 */
void light_off(const char *color, const char *direction);

/**
 * @brief Keep light on certain amount of time
 *
 * @param[in] color Traffic Light color
 * @param[in] direction Direction for LED corresponding to Traffic Light
 * @param[in] time Amount to keep light on for
 */
void light_solid(const char *color, const char *direction, float64_t time);

/**
 * @brief Flash light on certain number of times
 *
 * @param[in] color Traffic Light color
 * @param[in] direction Direction for LED corresponding to Traffic Light
 * @param[in] flash_count Number of times to turn light off -> on and viseversa
 */
void light_flash(const char *color, const char *direction, int flash_count);
#endif /* LED_CONTROL_H */
