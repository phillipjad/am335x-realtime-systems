#include "app_config.h"

#include <string.h>

#include "logger.h"
#include "project_constants.h"
#include "user_input.h"

#define CONFIG_CONTENT_MAX_LENGTH (4096U)
#define CONFIG_KEY_VALUE_DELIMITER ("=")

/*--------------------------------------
 * Static Function: get_config_file_path
 *--------------------------------------*/
static void get_config_file_path(char *config_path, size_t config_path_len) {
	if ((strlen(CONFIG_FILENAME) - 1U) > (MAX_FILENAME_LENGTH)) {
		LOG_AND_EXIT("Filename larger than maximum possible length: %zu > %u", strlen(CONFIG_FILENAME) - 1U, MAX_FILENAME_LENGTH);
	}
	char *result = getcwd(config_path, config_path_len);
	if (result == NULL) {
		LOG_AND_EXIT("Failed to get current working directory");
	}

	size_t cwd_len = strnlen(config_path, config_path_len);
	size_t remaining = config_path_len - cwd_len;
	size_t required_size = (strlen("/config") + strlen(CONFIG_FILENAME));
	if (remaining < required_size) {
		LOG_AND_EXIT("Remaining CWD string length less than required: (%zu) < (%zu)", remaining, required_size);
	}
	char *tmp = config_path;
	/* We've already verified that the remaining buffer is large enough, can void out return */
	(void)snprintf(&tmp[cwd_len], remaining, "/config%s", CONFIG_FILENAME);
}


/*--------------------------------------
 * Static Function: read_config_file_contents
 *--------------------------------------*/
static void read_config_file_contents(const char *config_path, char *file_content_buffer, size_t file_content_buffer_len) {
	FILE *cfg_file = fopen(config_path, "r");
	if (cfg_file == NULL) {
		LOG_AND_EXIT("Failed to open config file at %s", config_path);
	}

	size_t bytes_read = fread((void *)file_content_buffer, 1U, file_content_buffer_len, cfg_file);
	/* Check if we got data */
	if (bytes_read == 0U) {
		LOG_AND_EXIT("Read 0 bytes from configuration file at %s", config_path);
	}
	/* Check if we had a read error */
	if (ferror(cfg_file) != 0) {
		LOG_AND_EXIT("Failed to read config file at %s", config_path);
	}

	(void)fclose(cfg_file);
}

/*--------------------------------------
 * Static Function: parse_config_file_line
 *--------------------------------------*/
static int32_t parse_config_file_line(const char *line, configuration_items_t *config) {
	char line_copy[255] = { 0 };
	(void)strcpy(line_copy, line);
	char *saveptr = NULL;
	const char *key = strtok_r(line_copy, CONFIG_KEY_VALUE_DELIMITER, &saveptr);
	if (key == NULL) {
		return STATUS_FAIL;
	}
	const char *value = strtok_r(NULL, CONFIG_KEY_VALUE_DELIMITER, &saveptr);
	if (value == NULL) {
		return STATUS_FAIL;
	}

	if (strcmp(key, EAST_BUTTON_GPIO_KEY) == 0) {
		return parse_input_to_uint8(value, &config->gpio_layout.east_button);
	} else if (strcmp(key, WEST_BUTTON_GPIO_KEY) == 0) {
		return parse_input_to_uint8(value, &config->gpio_layout.west_button);
	} else if (strcmp(key, LED_1_GPIO_KEY) == 0) {
		return parse_input_to_uint8(value, &config->gpio_layout.led_1);
	} else if (strcmp(key, LED_2_GPIO_KEY) == 0) {
		return parse_input_to_uint8(value, &config->gpio_layout.led_2);
	} else if (strcmp(key, SERVO_GPIO_PIN) == 0) {
		return parse_input_to_uint8(value, &config->gpio_layout.servo);
	} else {
		LOG("Received unknown config key: %s", key);
		return STATUS_FAIL;
	}
}

/*--------------------------------------
 * Static Function: parse_config_file_contents
 *--------------------------------------*/
static void parse_config_file_contents(const char *config_content, configuration_items_t *config) {
	/* Make copy because strtok is considered a mutating function */
	char config_content_copy[CONFIG_CONTENT_MAX_LENGTH + 1U] = { 0 };
	(void)strcpy(config_content_copy, config_content);

	/* Tokenize each line */
	char *saveptr = NULL;
	char *line = strtok_r(config_content_copy, "\n", &saveptr);
	while (line != NULL) {
		/* We should ignore comment lines */
		if (line[0] != '#') {
			int32_t result = parse_config_file_line(line, config);
			if (result != STATUS_SUCCESS) {
				LOG_AND_EXIT("Failed to parse config line: (%s)", line);
			}
		}
		line = strtok_r(NULL, "\n", &saveptr);
	}
}

/*--------------------------------------
 * Function: load_app_config
 *--------------------------------------*/
void load_app_config(configuration_items_t *config) {
	char config_path[MAX_FILE_PATH_LENGTH + 1U] = { 0 };
	char config_content[CONFIG_CONTENT_MAX_LENGTH + 1U] = { 0 };
	get_config_file_path(config_path, MAX_FILE_PATH_LENGTH);
	LOG("Reading application configuration from %s", config_path);
	read_config_file_contents(config_path, config_content, CONFIG_CONTENT_MAX_LENGTH);
	parse_config_file_contents(config_content, config);
}
