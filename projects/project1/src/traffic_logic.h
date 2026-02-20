#ifndef TRAFFIC_LOGIC_H
#define TRAFFIC_LOGIC_H

#include "project_types.h"

/**
 * @brief Gracefully shuts down the system
 */
void handle_shutdown(void);

/**
 * @brief Runs the applications' traffic signal logic
 */
void run_traffic_signal(uint16_t green_light_time, gpio_layout_t light_pins);

#endif /* TRAFFIC_LOGIC_H */
