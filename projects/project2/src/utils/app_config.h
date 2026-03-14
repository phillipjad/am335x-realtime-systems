#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#include <unistd.h>

#include "project_types.h"

/* Config file key definitions */
#define EAST_BUTTON_GPIO_KEY ("EAST_BUTTON_GPIO")
#define WEST_BUTTON_GPIO_KEY ("WEST_BUTTON_GPIO")
#define LED_1_GPIO_KEY ("LED_1_GPIO")
#define LED_2_GPIO_KEY ("LED_2_GPIO")
#define SERVO_GPIO_PIN ("SERVO_GPIO")

/* Config file key definitions */

/**
 * @brief Reads the configuration file for the application. Fails fast if there is any error
 */
void load_app_config(configuration_items_t *config);

#endif /* APP_CONFIG_H */
