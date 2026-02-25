#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include "project_types.h"

static void gpio_map_init();

static void gpio_map_close();

static volatile uint32_t *get_gpio_base(uint8_t pin);

void gpio_set(uint8_t pin, bool value);

void gpio_set_direction_out(uint8_t pin);

static inline volatile uint32_t *reg32(volatile uint8_t *base, uint32_t off);

#endif /* GPIO_CONTROL_H */
