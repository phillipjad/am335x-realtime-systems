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

static global_values_t *shared_info = NULL;

/*--------------------------------------
 * Static Function: handle_clear
 *--------------------------------------*/
static void handle_clear(void) {
	bool cleared = false;
	pthread_mutex_lock(&shared_info->mutex);
	if (shared_info->current_state == STATE_FAIL_SAFE) {
		shared_info->current_state = STATE_IDLE;
		shared_info->current_direction = DIRECTION_NONE;
		cleared = true;
		pthread_cond_broadcast(&shared_info->cv);
	}
	pthread_mutex_unlock(&shared_info->mutex);
	if (cleared) {
		LOG("Supervisor has activated clear from fail-safe mode. Resetting railroad crossing...");
	} else {
		LOG("Not in fail-safe mode, disregarding input.");
	}
}

#ifndef NDEBUG
/*--------------------------------------
 * Static Function: handle_debug_east
 *--------------------------------------*/
static void handle_debug_east(void) {
	atomic_store(&shared_info->debug_east_pending, true);
	LOG("[DEBUG] Simulating EAST button press.");
}

/*--------------------------------------
 * Static Function: handle_debug_west
 *--------------------------------------*/
static void handle_debug_west(void) {
	atomic_store(&shared_info->debug_west_pending, true);
	LOG("[DEBUG] Simulating WEST button press.");
}

/*--------------------------------------
 * Static Function: handle_debug_help
 *--------------------------------------*/
static void handle_debug_help(void) {
	LOG("[DEBUG] Available commands:");
	LOG("[DEBUG]   clear, c                    - Clear fail-safe state (supervisor)");
	LOG("[DEBUG]   e, E, east, East, EAST      - Simulate EAST button press");
	LOG("[DEBUG]   w, W, west, West, WEST      - Simulate WEST button press");
	LOG("[DEBUG]   h, H, help                  - Show this help message");
}
#endif /* NDEBUG */

/*--------------------------------------
 * Static Function: handle_supervisor_input
 *--------------------------------------*/
static void handle_supervisor_input(void) {
	static struct pollfd poll_stdin = { .fd = STDIN_FILENO, .events = POLLIN, .revents = 0 };
	char input_buffer[USER_INPUT_MAX_LEN + 1U];

	// Poll every 100ms; if poll fails, will retry next call
	int32_t poll_result = poll(&poll_stdin, 1, 100);
	if (poll_result > 0 && (poll_stdin.revents & POLLIN)) {
		if (fgets(input_buffer, USER_INPUT_MAX_LEN, stdin) != NULL) {
			if (strcmp(input_buffer, "clear\n") == 0 || strcmp(input_buffer, "c\n") == 0) {
				handle_clear();
			}
#ifndef NDEBUG
			else if (strcmp(input_buffer, "e\n") == 0 || strcmp(input_buffer, "E\n") == 0 || strcmp(input_buffer, "east\n") == 0 ||
			    strcmp(input_buffer, "East\n") == 0 || strcmp(input_buffer, "EAST\n") == 0) {
				handle_debug_east();
			} else if (strcmp(input_buffer, "w\n") == 0 || strcmp(input_buffer, "W\n") == 0 || strcmp(input_buffer, "west\n") == 0 ||
			    strcmp(input_buffer, "West\n") == 0 || strcmp(input_buffer, "WEST\n") == 0) {
				handle_debug_west();
			} else if (strcmp(input_buffer, "help\n") == 0 || strcmp(input_buffer, "h\n") == 0 || strcmp(input_buffer, "H\n") == 0) {
				handle_debug_help();
			} else {
				/* MISRA requires else */
			}
#endif /* NDEBUG */
		}
	}
}

/*--------------------------------------
 * Function: supervisor_input_thread_entry
 *--------------------------------------*/
void *supervisor_input_thread_entry(void *args) {
	LOG("Supervisor thread started.");
	shared_info = (global_values_t *)args;

#ifndef NDEBUG
	handle_debug_help();
#endif /* NDEBUG */

	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		handle_supervisor_input();
	}

	LOG("Shutting down supervisor thread...");
	return NULL;
}
