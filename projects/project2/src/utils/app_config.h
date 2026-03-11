#ifndef APP_CONFIG_H
#define APP_CONFIG_H
#include <unistd.h>

#include "project_constants.h"
#include "project_types.h"

/* Config file key definitions */

/**
 * @brief Reads the configuration file for the application. Fails fast if there is any error
 */
void load_app_config(configuration_items_t *config);

#endif /* APP_CONFIG_H */
