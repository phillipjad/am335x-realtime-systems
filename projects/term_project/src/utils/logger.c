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
	written += strftime(&LOG_PATH[written], MAX_FILE_PATH_LENGTH - written, "%H_%M_%S/", tm);
	if (written > MAX_FILE_PATH_LENGTH) {
		LOG_AND_EXIT("File path length larger than limit: %zu > %u", written, MAX_FILE_PATH_LENGTH);
	}
	assert_directory_exists(LOG_PATH, 0755);

	log_queue_t *logger = &app_global->logger;
	int32_t result = pthread_mutex_init(&logger->log_mu, NULL);
	if (result != STATUS_SUCCESS) {
		LOG_AND_EXIT("Failed to init logger mutex");
	}

	return;
}

/*--------------------------------------
 * Function: project_log
 *--------------------------------------*/
inline void project_log(FILE *stream, bool include_newline, const char *filename, uint32_t line_no, const char *format, ...) {
	/* TODO: Add log level logic */
	char output_buffer[MAX_LOG_LEN + 1U] = { 0 };
	/* Get current time */
	struct timespec curr_time = { 0 };
	int32_t result = clock_gettime(CLOCK_REALTIME, &curr_time);
	if (result != 0) {
		printf("Failed to get timestamp");
		exit(EXIT_FAILURE);
	}
	int64_t microseconds = curr_time.tv_nsec / NSEC_PER_USEC;
	int32_t used = snprintf(output_buffer, MAX_LOG_LEN, "%" PRIdMAX ".%.6" PRId64, (intmax_t)curr_time.tv_sec, microseconds);
	const char *file_of_interest = NULL;
	if (strrchr(filename, '/') != NULL) {
		char *file_of_interest_with_slash = strrchr(filename, (int32_t)'/');
		/* Get rid of slash */
		++file_of_interest_with_slash;

		/* Now assign */
		file_of_interest = file_of_interest_with_slash;
	} else {
		file_of_interest = filename;
	}
	used += snprintf(&output_buffer[used], (MAX_LOG_LEN - (size_t)used), "[%s:%d]: ", file_of_interest, line_no);
	va_list args;
	va_start(args, format);
	used += vsnprintf(&output_buffer[used], (MAX_LOG_LEN - (size_t)used), format, args);
	va_end(args);
	if (include_newline) {
		used += snprintf(&output_buffer[used], (MAX_LOG_LEN - (size_t)used), "\n");
	}

	/* Immediately flush after printing to prevent interleaved output */
	printf("%s", output_buffer);
	(void)fflush(stream);
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
	LOG("It took %" PRIdMAX "ms to %s", (intmax_t)time_diff, action);
}
