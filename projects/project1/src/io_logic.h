#ifndef IO_LOGIC_H
#define IO_LOGIC_H
#include "project_types.h"

#define PIN_ON (1)
#define PIN_OFF (0)

/**
 * @brief Signals \p gpio_pin by writing \p value to it through sysfs GPIO control
 *
 * @param[in] gpio_pin The GPIO pin to signal
 * @param[in] value The value to signal to \p gpio_pin
 */
void signal_gpio(uint8_t gpio_pin, int8_t value);

#endif /* IO_LOGIC_H */
