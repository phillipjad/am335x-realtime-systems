#include <stdarg.h>
#include <string.h>

/* Local project includes after system libraries */
#include "logger.h"
#include "project_types.h"

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
	fprintf(stream, "[%s:%d]: ", filename, line_no);
	va_list args;
	va_start(args, format);
	vfprintf(stream, format_with_newline, args);
	va_end(args);
}
