#include "user_input.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

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
		LOG("User input is out of bounds for uint16_t: %" PRId64 " not within range 0 <= X <= %d", value, UINT8_MAX);
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
		LOG("User input is out of bounds for uint16_t: %" PRId64 " not within range 0 <= X <= %d", value, UINT16_MAX);
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

/*--------------------------------------
 * Function: user_input_thread_entry
 *--------------------------------------*/
void *user_input_thread_entry(void *args) {
	LOG("Supervisor thread started.");
	global_values_t *shared_info = (global_values_t *)args;
	char input_buffer[USER_INPUT_MAX_LEN + 1U];
	bool clear_fail_safe = false;

	// Poll setup
	struct pollfd poll_stdin = { 0 };
	poll_stdin.fd = STDIN_FILENO;
	poll_stdin.events = POLLIN;

	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		// Start polling every 100ms
		int32_t poll_fd = poll(&poll_stdin, 1, 100);

		// Check for input with poll length
		if (poll_fd > 0 && (poll_stdin.revents & POLLIN)) {
			if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
				// Check if c or clear were typed
				if (strcmp(input_buffer, "clear\n") == 0 || strcmp(input_buffer, "c\n") == 0) {
					// Grab lock
					pthread_mutex_lock(&shared_info->mutex);
					if (shared_info->current_state == STATE_FAIL_SAFE) {
						shared_info->current_state = STATE_IDLE;
						shared_info->current_direction = DIRECTION_NONE;
						clear_fail_safe = true;
						pthread_cond_broadcast(&shared_info->cv);
					} else {
						clear_fail_safe = false;
					}
					// Release lock
					pthread_mutex_unlock(&shared_info->mutex);
					// Log outside critical section
					if (clear_fail_safe) {
						LOG("Supervisor has activated clear from fail-safe mode. Resetting railroad crossing...");
					} else {
						LOG("Not in fail-safe mode, disregarding input.");
					}
				}
			}
		}
	}

	LOG("Shutting down supervisor thread...");
	return NULL;
}
