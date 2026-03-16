#include "supervisor_input.h"

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

/*--------------------------------------
 * Function: supervisor_input_thread_entry
 *--------------------------------------*/
void *supervisor_input_thread_entry(void *args) {
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
			if (fgets(input_buffer, USER_INPUT_MAX_LEN, stdin) != NULL) {
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
