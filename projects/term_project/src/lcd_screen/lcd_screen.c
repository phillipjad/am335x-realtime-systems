#include "lcd_screen.h"
#include <unistd.h>

/* Local project includes after system libraries */
#include "heartbeat.h"
#include "logger.h"
#include "project_types.h"

static global_values_t *shared_info = NULL;

void *lcd_screen_thread_entry(void *arg) {
	LOG("Starting LCD screen thread");
	shared_info = (global_values_t *)arg;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		sleep(2U);
		increment_heartbeat(shared_info, LCD_SCREEN);
	}
	LOG("Shutting down LCD screen thread");
	return NULL;
}
