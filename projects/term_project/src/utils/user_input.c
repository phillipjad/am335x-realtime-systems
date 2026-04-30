#include "user_input.h"

#include <errno.h>
#include <string.h>

/* Local project includes after system libraries */
#include "logger.h"

#define BASE_DEC (10)

/*--------------------------------------
 * Function: parse_input_to_uint8
 *--------------------------------------*/
int32_t parse_input_to_uint8(const char *input_buffer, uint8_t *output) {
	char *endptr = NULL;
	errno = 0;
	int64_t value = strtol(input_buffer, &endptr, BASE_DEC);
	/* Check for errno */
	if (errno != 0) {
		LOG(NUM_THREADS, "Experienced error while parsing user input to uint8_t: %s", strerror(errno));
		return STATUS_FAIL;
	}
	/* Check that input was actually parsed */
	if (endptr == input_buffer) {
		LOG(NUM_THREADS, "Failed to parse input: %s", input_buffer);
		return STATUS_FAIL;
	}

	if ((value > UINT8_MAX) || (value < 0)) {
		LOG(NUM_THREADS, "User input is out of bounds for uint16_t: %" PRId64 " not within range 0 <= X <= %d", value, UINT8_MAX);
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
		LOG(NUM_THREADS, "Experienced error while parsing user input to uint16_t: %s", strerror(errno));
		return STATUS_FAIL;
	}

	/* Check that input was actually parsed */
	if (endptr == input_buffer) {
		LOG(NUM_THREADS, "Failed to parse input: %s", input_buffer);
		return STATUS_FAIL;
	}

	if ((value > UINT16_MAX) || (value < 0)) {
		LOG(NUM_THREADS, "User input is out of bounds for uint16_t: %" PRId64 " not within range 0 <= X <= %d", value, UINT16_MAX);
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
		LOG(NUM_THREADS, "Experienced error while parsing user input to float64_t: %s", strerror(errno));
		return STATUS_FAIL;
	}
	/* Check that input was actually parsed */
	if (endptr == input_buffer) {
		LOG(NUM_THREADS, "Failed to parse input: %s", input_buffer);
		return STATUS_FAIL;
	}

	*output = value;

	return STATUS_SUCCESS;
}

/*--------------------------------------
 * Function: parse_pwm_input
 *--------------------------------------*/
int32_t parse_pwm_input(const char *input_buffer, uint8_t *chip, char *channel) {
	// Parsing uint8_t value
	char *endptr = NULL;
	errno = 0;

	// Parse chip value
	int64_t value = strtol(input_buffer, &endptr, BASE_DEC);
	/* Check for errno */
	if (errno != 0) {
		LOG(NUM_THREADS, "Experienced error while parsing pwm chip input value to uint8_t: %s", strerror(errno));
		return STATUS_FAIL;
	}
	/* Check that input was actually parsed */
	if (endptr == input_buffer) {
		LOG(NUM_THREADS, "Failed to parse input: %s", input_buffer);
		return STATUS_FAIL;
	}

	// Check if value is either 1 or 2 for EHRPWM[1, 2][a, b]
	if ((value != 1) && (value != 2)) {
		LOG(NUM_THREADS, "Parsed chip value %" PRId64 " is out of bounds range 1 <= X <= 2", value);
		return STATUS_FAIL;
	}

	// Parse to find the letter after the number (either a or b)
	char channel_char = *endptr;
	if ((channel_char != 'a') && (channel_char != 'b') && (channel_char != 'A') && (channel_char != 'B')) {
		LOG(NUM_THREADS, "Invalid pwm channel character: %c. Should be either 'a' or 'b'.", channel_char);
		return STATUS_FAIL;
	}

	*chip = (uint8_t)value;
	*channel = channel_char;
	return STATUS_SUCCESS;
}
