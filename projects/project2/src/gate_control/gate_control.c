#include "gate_control.h"

#include <pthread.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"
#include "servo_controller.h"

/*--------------------------------------
 * Function: gate_control_thread_entry
 *--------------------------------------*/
void *gate_control_thread_entry(void *arg) {
	LOG("Starting gate control thread");
	global_values_t *shared_info = (global_values_t *)arg;
	// Starting state
	state_t servo_state = STATE_IDLE;
	// Grab lock
	pthread_mutex_lock(&shared_info->mutex);
	// Assign current starting state
	servo_state = shared_info->current_state;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		// While state is same as initializing state, wait on cv
		while (shared_info->current_state == servo_state && !atomic_load(&shared_info->is_shutdown_requested)) {
			pthread_cond_wait(&shared_info->cv, &shared_info->mutex);
		}
		// Check if shutdown requested or state changed from idle
		if (!atomic_load(&shared_info->is_shutdown_requested)) {
			// Snapshot of state
			state_t snapshot_state = shared_info->current_state;
			// Update current servo state
			servo_state = snapshot_state;
			// Release lock
			pthread_mutex_unlock(&shared_info->mutex);
			if (snapshot_state == STATE_IDLE) {
				servo_raise();
				LOG("Gate read IDLE state, raising gate.");
			} else if (snapshot_state == STATE_ACTIVE) {
				servo_lower();
				LOG("Gate read STATE_ACTIVE state, lowering gate.");
			} else if (snapshot_state == STATE_FAIL_SAFE) {
				servo_lower();
				LOG("Gate read FAIL_STATE state, lowering gate.");
			} else if (snapshot_state == STATE_CLEARING) {
				LOG("Gate read CLEARING_STATE state");
				// Sleep for 1 second as per requirements
				sleep(1);
				servo_raise();
				LOG("Waited and raising gate.");
				// Grab lock
				pthread_mutex_lock(&shared_info->mutex);

				// Check if state changed right before grabbing mutex
				if (shared_info->current_state == STATE_CLEARING) {
					shared_info->current_state = STATE_IDLE;
					shared_info->current_direction = DIRECTION_NONE;
					pthread_cond_broadcast(&shared_info->cv);
				}
				continue;
			}
			// Grab lock
			pthread_mutex_lock(&shared_info->mutex);
		} else {
			break;
		}
	}
	// Release lock
	pthread_mutex_unlock(&shared_info->mutex);
	LOG("Shutting down gate control thread");
	return NULL;
}
