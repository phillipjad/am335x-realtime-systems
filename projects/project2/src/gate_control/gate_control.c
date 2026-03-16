#include "gate_control.h"

#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

/*--------------------------------------
 * Function: gate_control_thread_entry
 *--------------------------------------*/
void *gate_control_thread_entry(void *arg) {
	LOG("Starting gate control thread");
	global_values_t *shared_info = (global_values_t *)arg;
	// Assign current starting state which will be STATE_IDLE
	state_t servo_state = shared_info->current_state;
	// Grab lock
	pthread_mutex_lock(&shared_info->mutex);
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
				// TODO: SERVO UP FUNCTION
				LOG("Gate read IDLE state, raising gate.");
			} else if (snapshot_state == STATE_ACTIVE) {
				// TODO: SERVO DOWN FUNCTION
				LOG("Gate read STATE_ACTIVE state, lowering gate.");
			} else if (snapshot_state == STATE_FAIL_SAFE) {
				// TODO: SERVO DOWN FUNCTION
				LOG("Gate read FAIL_STATE state, lowering gate.");
			} else if (snapshot_state == STATE_CLEARING) {
				LOG("Gate read CLEARING_STATE state");
				// Sleep for 1 second as per requirements
				sleep(1);
				LOG("Waited and raising gate.");
				// TODO: SERVO UP FUNCTION
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
