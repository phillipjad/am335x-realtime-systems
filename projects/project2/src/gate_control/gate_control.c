#include "gate_control.h"

#include <pthread.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_types.h"
#include "servo_controller.h"

static global_values_t *shared_info = NULL;

static void handle_gate_logic(void) {
	/* We want to track what the last state we acted on is */
	static state_t last_operated_state = STATE_INVALID;
	/* Grab state snapshot */
	pthread_mutex_lock(&shared_info->mutex);
	state_t current_state = shared_info->current_state;
	// While state is same as initializing state, wait on cv
	while ((current_state == last_operated_state) && (!atomic_load(&shared_info->is_shutdown_requested))) {
		pthread_cond_wait(&shared_info->cv, &shared_info->mutex);
	}
	/* Grab updated state snapshot while we have the mutex */
	current_state = shared_info->current_state;
	pthread_mutex_unlock(&shared_info->mutex);

	// Check if shutdown requested or state changed from idle
	if (!atomic_load(&shared_info->is_shutdown_requested)) {
		if (current_state == STATE_IDLE) {
			servo_raise();
			LOG("Gate read IDLE state, raising gate.");
		} else if (current_state == STATE_ACTIVE) {
			servo_lower();
			LOG("Gate read STATE_ACTIVE state, lowering gate.");
		} else if (current_state == STATE_FAIL_SAFE) {
			servo_lower();
			LOG("Gate read FAIL_STATE state, lowering gate.");
		} else if (current_state == STATE_CLEARING) {
			LOG("Gate read CLEARING_STATE state");
			// Sleep for 1 second as per requirements
			sleep(1);
			servo_raise();
			LOG("Waited and raising gate.");
			// Grab lock
			return;
		}
		last_operated_state = current_state;
	} else {
		return;
	}
}

/*--------------------------------------
 * Function: gate_control_thread_entry
 *--------------------------------------*/
void *gate_control_thread_entry(void *arg) {
	LOG("Starting gate control thread");
	shared_info = (global_values_t *)arg;
	// Assign current starting state
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		handle_gate_logic();
	}
	LOG("Shutting down gate control thread");
	return NULL;
}
