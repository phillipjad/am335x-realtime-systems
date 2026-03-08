#include "user_input.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Local project includes after system libraries */
#include "logger.h"

#define BASE_DEC (10)

/*--------------------------------------
 * Function: get_user_input
 *--------------------------------------*/
int32_t get_user_input(char *input_buffer, size_t input_buffer_len, const char *prompt) {
	LOG_WITHOUT_NEWLINE("%s: ", prompt);
	char *return_val = fgets(input_buffer, input_buffer_len, stdin);
	return return_val != NULL ? STATUS_SUCCESS : STATUS_FAIL;
}

/*--------------------------------------
 * Function: parse_input_to_uint8
 *--------------------------------------*/
int32_t parse_input_to_uint8(const char *input_buffer, uint8_t *output) {
	char *endptr = NULL;
	errno = 0;
	int64_t value = strtol(input_buffer, &endptr, BASE_DEC);
	/* Check for errno */
	if (errno != 0) {
		LOG("Experienced error while parsing user input to uint8_t: %s", strerror(errno));
		return STATUS_FAIL;
	}
	/* Check that input was actually parsed */
	if (endptr == input_buffer) {
		LOG("Failed to parse input: %s", input_buffer);
		return STATUS_FAIL;
	}

	if ((value > UINT8_MAX) || (value < 0)) {
		LOG("User input is out of bounds for uint8_t: %ld not within range 0 <= X <= %d", value, UINT8_MAX);
		return STATUS_FAIL;
	}

	*output = (uint8_t)value;

	return STATUS_SUCCESS;
}

/*--------------------------------------
 * Function: parse_input_to_uint16
 *--------------------------------------*/
int32_t parse_input_to_uint16(const char *input_buffer, uint16_t *output) {
	char *endptr = NULL;
	errno = 0;
	int64_t value = strtol(input_buffer, &endptr, BASE_DEC);
	/* Check for errno */
	if (errno != 0) {
		LOG("Experienced error while parsing user input to uint16_t: %s", strerror(errno));
		return STATUS_FAIL;
	}

	/* Check that input was actually parsed */
	if (endptr == input_buffer) {
		LOG("Failed to parse input: %s", input_buffer);
		return STATUS_FAIL;
	}

	if ((value > UINT16_MAX) || (value < 0)) {
		LOG("User input is out of bounds for uint16_t: %ld not within range 0 <= X <= %d", value, UINT16_MAX);
		return STATUS_FAIL;
	}

	*output = (uint16_t)value;

	return STATUS_SUCCESS;
}

/*--------------------------------------
 * Function: parse_input_to_float64
 *--------------------------------------*/
int32_t parse_input_to_float64(const char *input_buffer, double *output) {
	char *endptr = NULL;
	errno = 0;
	float64_t value = strtod(input_buffer, &endptr);
	/* Check for errno */
	if (errno != 0) {
		LOG("Experienced error while parsing user input to float64_t: %s", strerror(errno));
		return STATUS_FAIL;
	}
	/* Check that input was actually parsed */
	if (endptr == input_buffer) {
		LOG("Failed to parse input: %s", input_buffer);
		return STATUS_FAIL;
	}

	*output = value;

	return STATUS_SUCCESS;
}
