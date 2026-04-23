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
 * @param[in] train_direction Incoming train direction
 * @return void
 */
//void sensor_monitoring(direction_t train_direction);

/**
 * @brief Fail-safe for train buttons
 *
 * @return void
 */
void failsafe_timeout(void);

#endif /* SENSOR_MONITORING_H */
