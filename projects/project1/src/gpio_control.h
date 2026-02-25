#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include "project_types.h"

void gpio_map_init();

void gpio_map_close();

void gpio_set(uint8_t pin, bool value);

void gpio_set_direction_out(uint8_t pin);

#endif /* GPIO_CONTROL_H */
