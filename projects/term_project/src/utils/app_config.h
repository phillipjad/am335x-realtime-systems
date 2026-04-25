#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#include <unistd.h>

#include "project_types.h"

/* Config file key definitions */
#define TARGET_TEMP_LED_GPIO_KEY ("TARGET_TEMP_LED_GPIO")
#define SYSTEM_OK_LED_GPIO_KEY ("SYSTEM_OK_LED_GPIO")
#define SYSTEM_FAIL_LED_GPIO_KEY ("SYSTEM_FAIL_LED_GPIO")
#define LCD_IC2_NUMBER_KEY ("LCD_IC2_NUMBER")
#define SERVO_GPIO_PIN ("SERVO_GPIO")
#define POTENTIOMETER_AIN_PIN ("POTENTIOMETER_AIN")

/* Config file key definitions */

/**
 * @brief Reads the configuration file for the application. Fails fast if there is any error
 */
void load_app_config(configuration_items_t *config);

#endif /* APP_CONFIG_H */
