#ifndef SUPERVISOR_INPUT_H
#define SUPERVISOR_INPUT_H
#include "project_types.h"

/**
 * @brief Thread entry point for fail-safe supervisor user input_buffer
 *
 * @param[in] args Shared global values pointer
 * @param[out] void* returns NULL
 */
void *supervisor_input_thread_entry(void *args);

#endif /* SUPERVISOR_INPUT_H */
