#ifndef PROJECT_TYPES_H
#define PROJECT_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STATUS_SUCCESS (0)
#define STATUS_FAIL (-1)

typedef double float64_t;

typedef struct {
	uint8_t green_light_ns;  /**< Pin for green light on north/south */
	uint8_t green_light_ew;  /**< Pin for green light on east/west */
	uint8_t yellow_light_ns; /**< Pin for yellow light on north/south */
	uint8_t yellow_light_ew; /**< Pin for yellow light on east/west */
	uint8_t red_light_ns;    /**< Pin for red light on north/south */
	uint8_t red_light_ew;    /**< Pin for red light on east/west */
} gpio_layout_t;

typedef struct {
	uint16_t green_light_duration_s;
	gpio_layout_t gpio_layout;
} configuration_items_t;

#endif /* PROJECT_TYPES_H */
