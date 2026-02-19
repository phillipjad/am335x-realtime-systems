#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#include <unistd.h>

#include "project_constants.h"
#include "project_types.h"

/* Config file key definitions */
#define GREEN_LIGHT_DURATION_KEY ("GREEN_LIGHT_DURATION_M")
#define GREEN_LIGHT_PIN_NS_KEY ("GREEN_LIGHT_PIN_NS")
#define YELLOW_LIGHT_PIN_NS_KEY ("YELLOW_LIGHT_PIN_NS")
#define RED_LIGHT_PIN_NS_KEY ("RED_LIGHT_PIN_NS")
#define GREEN_LIGHT_PIN_EW_KEY ("GREEN_LIGHT_PIN_EW")
#define YELLOW_LIGHT_PIN_EW_KEY ("YELLOW_LIGHT_PIN_EW")
#define RED_LIGHT_PIN_EW_KEY ("RED_LIGHT_PIN_EW")

/**
 * @brief Reads the configuration file for the application. Fails fast if there is any error
 * 
 * @param[out] config A struct holding all configuration items to populate
 */
void load_app_config(configuration_items_t *config);

#endif /* APP_CONFIG_H */