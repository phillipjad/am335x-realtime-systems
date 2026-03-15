#ifndef USER_INPUT_H
#define USER_INPUT_H
#include "project_types.h"

/**
 * @brief Prompt the user and ingest the response
 *
 * @param[out] input_buffer A buffer to save the user response into
 * @param[in] input_buffer_len The length of \p input_buffer_len
 * @param[in] prompt The prompt to provide to the user
 * @return int32_t Returns STATUS_SUCCESS on success and STATUS_FAIL on failure
 */
int32_t get_user_input(char *input_buffer, size_t input_buffer_len, const char *prompt);

/**
 * @brief Parses the string in \p input_buffer into a uint8_t and stores in \p output
 *
 * @param[in] input_buffer A buffer to save the user response into
 * @param[out] output A uint8_t value to store the parsed integer value of \p input_buffer
 * @return int32_t Returns STATUS_SUCCESS on success and STATUS_FAIL on failure
 */
int32_t parse_input_to_uint8(const char *input_buffer, uint8_t *output);

/**
 * @brief Parses the string in \p input_buffer into a uint16_t and stores in \p output
 *
 * @param[in] input_buffer A buffer to save the user response into
 * @param[out] output A uint16_t value to store the parsed integer value of \p input_buffer
 * @return int32_t Returns STATUS_SUCCESS on success and STATUS_FAIL on failure
 */
int32_t parse_input_to_uint16(const char *input_buffer, uint16_t *output);

/**
 * @brief Parses the string in \p input_buffer into a float64_t and stores in \p output
 *
 * @param[in] input_buffer A buffer to save the user response into
 * @param[out] output A float64_t value to store the parsed float value of \p input_buffer
 * @return int32_t Returns STATUS_SUCCESS on success and STATUS_FAIL on failure
 */
int32_t parse_input_to_float64(const char *input_buffer, float64_t *output);

/**
 * @brief Thread entry point for fail-safe supervisor user input_buffer
 *
 * @param[in] args Shared global values pointer
 * @param[out] void* returns NULL
 */
void *user_input_thread_entry(void *args);

#endif /* USER_INPUT_H */
