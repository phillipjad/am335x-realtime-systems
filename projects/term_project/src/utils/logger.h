#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>

/* Local project includes after system libraries */
#include "project_types.h" // IWYU pragma: keep | We are trying to pull stdint and stdbool from this intrinisically

/**
 * @brief printf-like logging macro
 */
#define LOG(tid, format, ...) project_log(tid, stdout, true, __FILE__, __LINE__, format, ##__VA_ARGS__)


/**
 * @brief printf-like logging macro, with no newline
 */
#define LOG_WITHOUT_NEWLINE(tid, format, ...) project_log(tid, stdout, false, __FILE__, __LINE__, format, ##__VA_ARGS__)

/**
 * @brief Panics the message in \p format to stderr and then exits with a non-zero status code
 */
#define LOG_AND_EXIT(format, ...)                                                      \
	project_log(NUM_THREADS, stderr, true, __FILE__, __LINE__, format, ##__VA_ARGS__); \
	exit(EXIT_FAILURE)

/**
 * @brief Logs the message in \p format alongside useful metadata such as the filename and lineno
 *
 * @param[in] tid The thread index of the calling thread
 * @param[in] stream The output stream to write to (stdout/stderr)
 * @param[in] include_newline Determines if a newline should be included in the log statement.
 * @param[in] filename The file that this function was called from
 * @param[in] line_no The line number where this function was called
 * @param[in] format LOG-like format string to use when printing
 * @param[in] ... Variadic arguments to print according to \p format
 */
void project_log(thread_index_e tid, FILE *stream, bool include_newline, const char *filename, uint32_t line_no, const char *format, ...)
    /* GCC format attribute for printf-style format checking */
    __attribute__((format(printf, 6, 7)));

/**
 * @brief Initializes the log handler by setting up all required directories and bootstrapping the queue structure.
 */
void init_log_handler(global_values_t *app_global);

/**
 * @brief Logs the time difference between \p t1 and \p t2 to do \p action
 *
 * @param[in] t1 The minuend of the difference
 * @param[in] t2 The subtrahend of the difference
 * @param[in] action The action that time is being diffed from. Should be a present-tense verb phrase
 */
void log_time_difference_ms(struct timespec t1, struct timespec t2, const char *action);

/**
 * @brief Returns the base path used for log file output.
 */
const char *get_log_base_path(void);

/** @brief Thread names indexed by thread_index_e, for use in log file naming. */
extern const char *const THREAD_NAMES[NUM_THREADS];

#endif /* LOGGER_H */
