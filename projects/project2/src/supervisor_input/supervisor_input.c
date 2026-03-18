#include "supervisor_input.h"

#include <pthread.h>
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
		// Not checking for when poll returns -1, since if poll fails, will retry poll in 100ms

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
#ifdef DEBUG
				else if (strcmp(input_buffer, "e\n") == 0 || strcmp(input_buffer, "E\n") == 0 ||
				    strcmp(input_buffer, "east\n") == 0 || strcmp(input_buffer, "East\n") == 0 ||
				    strcmp(input_buffer, "EAST\n") == 0) {
					atomic_store(&shared_info->debug_east_pending, true);
					LOG("[DEBUG] Simulating EAST button press.");
				} else if (strcmp(input_buffer, "w\n") == 0 || strcmp(input_buffer, "W\n") == 0 ||
				    strcmp(input_buffer, "west\n") == 0 || strcmp(input_buffer, "West\n") == 0 ||
				    strcmp(input_buffer, "WEST\n") == 0) {
					atomic_store(&shared_info->debug_west_pending, true);
					LOG("[DEBUG] Simulating WEST button press.");
				} else if (strcmp(input_buffer, "help\n") == 0 || strcmp(input_buffer, "h\n") == 0 ||
				    strcmp(input_buffer, "H\n") == 0) {
					LOG("[DEBUG] Available commands:");
					LOG("[DEBUG]   clear, c                    - Clear fail-safe state (supervisor)");
					LOG("[DEBUG]   e, E, east, East, EAST      - Simulate EAST button press");
					LOG("[DEBUG]   w, W, west, West, WEST      - Simulate WEST button press");
					LOG("[DEBUG]   h, H, help                  - Show this help message");
				}
#endif /* DEBUG */
			}
		}
	}

	LOG("Shutting down supervisor thread...");
	return NULL;
}
