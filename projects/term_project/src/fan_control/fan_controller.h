#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include "project_types.h"

void fan_controller_init(uint8_t fan_chip, char fan_channel);

int32_t fan_set_speed_pct(uint8_t percent);

void fan_shutdown(void);

#endif /* FAN_CONTROLLER_H */
