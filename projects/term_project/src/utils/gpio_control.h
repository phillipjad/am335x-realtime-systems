#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include "project_types.h"

/* GPIO Types */
#define GPIO_IN ("In")
#define GPIO_OUT ("Out")
#define GPIO_HIGH (true)
#define GPIO_LOW (false)

void gpio_map_init(void);

void gpio_map_close(void);

void gpio_set(uint8_t pin, bool value);

uint8_t gpio_read(uint8_t pin);

void gpio_clear(uint8_t pin);

void gpio_set_direction(uint8_t pin, const char *direction);

void gpio_set_direction_out(uint8_t pin);

void gpio_set_direction_in(uint8_t pin);

#endif /* GPIO_CONTROL_H */
