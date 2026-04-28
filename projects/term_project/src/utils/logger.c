#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

static char LOG_PATH[MAX_FILE_PATH_LENGTH + 1U] = { 0 };
static log_queue_t *g_log_queue = NULL;

const char *const THREAD_NAMES[NUM_THREADS] = { "vent_control", "log_handler", "lcd_screen", "temperature_sensor",
	"led", "potentiometer", "state_management", "fan_control" };

/**
 * @brief Asserts that path exists either through creation or pre-existance
 *
 * @param path[in] Path to check
 * @param mode The mode the create \p path at
 */
static void assert_directory_exists(const char *path, mode_t mode) {
	int32_t result = mkdir(path, mode);
	if ((result != STATUS_SUCCESS) && (errno != EEXIST)) {
		LOG_AND_EXIT("Failed to assert that %s directory exists", path);
	}
}

void init_log_handler(global_values_t *app_global) {
	/* Ensure the base of the log dir is present */
	assert_directory_exists(LOG_PARENT_DIR, 0755);
	assert_directory_exists(LOG_BASE_DIR, 0755);

	/* Ensure the directory for today is present */
	size_t written = snprintf(LOG_PATH, MAX_FILE_PATH_LENGTH, "%s/", LOG_BASE_DIR);
	struct timespec now_ts = { 0 };
	(void)clock_gettime(CLOCK_REALTIME, &now_ts);
	struct tm *tm = localtime(&now_ts.tv_sec);
	written += strftime(&LOG_PATH[written], MAX_FILE_PATH_LENGTH - written, "%m_%d_%Y/", tm);
	if (written > MAX_FILE_PATH_LENGTH) {
		LOG_AND_EXIT("File path length larger than limit: %zu > %u", written, MAX_FILE_PATH_LENGTH);
	}
	assert_directory_exists(LOG_PATH, 0755);

	/* Create the directory for this instance of the program */
	written += strftime(&LOG_PATH[written], MAX_FILE_PATH_LENGTH - written, "%I_%M_%S_%p/", tm);
	if (written > MAX_FILE_PATH_LENGTH) {
		LOG_AND_EXIT("File path length larger than limit: %zu > %u", written, MAX_FILE_PATH_LENGTH);
	}
	assert_directory_exists(LOG_PATH, 0755);

	g_log_queue = &app_global->logger;
	int32_t result = pthread_mutex_init(&g_log_queue->log_mu, NULL);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to init logger mutex");
	}
	result = pthread_cond_init(&g_log_queue->log_cv, NULL);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to init logger condition variable");
	}
	g_log_queue->head = 0U;
	g_log_queue->tail = 0U;
	g_log_queue->size = 0U;
}

const char *get_log_base_path(void) {
	return LOG_PATH;
}

/*--------------------------------------
 * Function: project_log
 *--------------------------------------*/
void project_log(thread_index_e tid, FILE *stream, bool include_newline, const char *filename, uint32_t line_no, const char *format, ...) {
	va_list args;
	va_start(args, format);

	if (stream == stderr) {
		/* Fatal path: format and write directly, bypassing the queue */
		char output_buffer[MAX_LOG_LEN + 1U] = { 0 };
		struct timespec curr_time = { 0 };
		(void)clock_gettime(CLOCK_REALTIME, &curr_time);
		int64_t microseconds = curr_time.tv_nsec / NSEC_PER_USEC;
		int32_t used = snprintf(output_buffer, MAX_LOG_LEN, "%" PRIdMAX ".%.6" PRId64, (intmax_t)curr_time.tv_sec, microseconds);
		const char *basename = strrchr(filename, '/');
		basename = (basename != NULL) ? (basename + 1) : filename;
		used += snprintf(&output_buffer[used], (MAX_LOG_LEN - (size_t)used), "[%s:%" PRIu32 "]: ", basename, line_no);
		used += vsnprintf(&output_buffer[used], (MAX_LOG_LEN - (size_t)used), format, args);
		if (include_newline) {
			(void)snprintf(&output_buffer[used], (MAX_LOG_LEN - (size_t)used), "\n");
		}
		fprintf(stderr, "%s", output_buffer);
		(void)fflush(stderr);
		va_end(args);
		return;
	}

	/* stdout path: wrap into log_queue_message_t and enqueue */
	log_queue_message_t msg = { 0 };
	msg.thread_id = tid;
	msg.include_newline = include_newline;
	msg.line_no = line_no;
	(void)strncpy(msg.filename, filename, MAX_FILENAME_LENGTH);
	(void)vsnprintf(msg.message, MAX_LOG_LEN, format, args);
	va_end(args);

	pthread_mutex_lock(&g_log_queue->log_mu);
	if (g_log_queue->size < MAX_LOG_QUEUE_CAPACITY) {
		g_log_queue->queue[g_log_queue->tail] = msg;
		g_log_queue->tail = (g_log_queue->tail + 1U) % MAX_LOG_QUEUE_CAPACITY;
		++g_log_queue->size;
		(void)pthread_cond_signal(&g_log_queue->log_cv);
		pthread_mutex_unlock(&g_log_queue->log_mu);
	} else {
		pthread_mutex_unlock(&g_log_queue->log_mu);
		fprintf(stderr, "WARN: log queue full, message dropped: %s\n", msg.message);
		(void)fflush(stderr);
	}
}

/*--------------------------------------
 * Function: log_time_difference_ms
 *--------------------------------------*/
void log_time_difference_ms(struct timespec t1, struct timespec t2, const char *action) {
	time_t t1_as_ms = 0L;
	t1_as_ms += t1.tv_sec * MSEC_PER_SEC;
	t1_as_ms += t1.tv_nsec / NSEC_PER_MSEC;

	time_t t2_as_ms = 0L;
	t2_as_ms += t2.tv_sec * MSEC_PER_SEC;
	t2_as_ms += t2.tv_nsec / NSEC_PER_MSEC;

	time_t time_diff = t1_as_ms - t2_as_ms;
	LOG(NUM_THREADS, "It took %" PRIdMAX "ms to %s", (intmax_t)time_diff, action);
}
