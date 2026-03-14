#ifndef SENSOR_MONITORING_H
#define SENSOR_MONITORING_H

#include "project_types.h"

/**
 * @brief Thread entry point for the sensor monitoring module
 *
 * @param[in] arg Thread arguments
 * @return void* Returns NULL
 */
void *sensor_monitoring_thread_entry(void *arg);

/**
 * @brief Handler for when button event is detected
 *
 * @param[in] shared_info Shared variables
 * @param[in] direction Incoming train direction
 * @return void
 */
void sensor_monitoring(global_values_t *shared_info, direction_t train_direction);

/**
 * @brief Fail-safe for train buttons
 *
 * @param[in] shared_info Shared variables
 * @return void
 */
void failsafe_timout(global_values_t *shared_info);

#endif /* SENSOR_MONITORING_H */
