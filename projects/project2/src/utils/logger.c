#include <stdarg.h>
#include <string.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_types.h"

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/*--------------------------------------
 * Function: project_log
 *--------------------------------------*/
inline void project_log(FILE *stream, bool include_newline, const char *filename, uint32_t line_no, const char *format, ...) {
	/* TODO: Add timestamp and log level logic */
	char format_with_newline[4096] = { 0 };
	strncpy(format_with_newline, format, 4095);
	if (include_newline) {
		/* TODO: This is kinda janky rn, but I don't want to put newlines in every log */
		format_with_newline[strlen(format)] = '\n';
	}
	const char *file_of_interest = NULL;
	if (strrchr(filename, (int32_t)'/') != NULL) {
		char *file_of_interest_with_slash = strrchr(filename, (int32_t)'/');
		file_of_interest = ++file_of_interest_with_slash;
	} else {
		file_of_interest = filename;
	}
	fprintf(stream, "[%s:%d]: ", file_of_interest, line_no);
	va_list args;
	va_start(args, format);
	/* Add lock around print and flush */
	pthread_mutex_lock(&log_mutex);
	vfprintf(stream, format_with_newline, args);
	/* Immediately flush to prevent intreleaved output */
	(void)fflush(stream);
	pthread_mutex_unlock(&log_mutex);
	va_end(args);
}
