#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include "project_types.h"

/**
 * @brief Function to intialize servo
 *
 * @param[in] shared_info Shared variables
 * @param[in] pin GPIO pin for servo
 * @return void* Returns NULL
 */
void sensor_init(global_values_t *shared_info, uint8_t pin);

/**
 * @brief Function to raise servo
 *
 * @return void* Returns NULL
 */
void servo_raise(void);

/**
 * @brief Function to lower servo
 *
 * @return void* Returns NULL
 */
void servo_lower(void);

/**
 * @brief Function to shutdown servo
 *
 * @return void* Returns NULL
 */
void servo_shutdown(void);


#endif /* SERVO_CONTROL_H */
