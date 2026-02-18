#include "user_input.h"

#include <stdio.h>

/* Local project includes after system libraries */
#include "logger.h"

int32_t get_user_input(char *input_buffer, size_t input_buffer_len, const char *prompt) {
	LOG_WITHOUT_NEWLINE("%s: ", prompt);
	char *return_val = fgets(input_buffer, input_buffer_len, stdin);
	return return_val != NULL ? STATUS_SUCCESS : STATUS_FAIL;
}
