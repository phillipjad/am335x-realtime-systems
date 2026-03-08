#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <stdint.h>

/* Local project includes after system libraries */
#include <project_types.h>

/**
 * @brief Registers SIGINT and SIGTERM signal handlers
 * @param[in,out] is_shutdown_requested An atomic boolean value to set when shutdown is signalled
 * @return int32_t Returns STATUS_SUCCESS on success and STATUS_FAIL on failure
 */
int32_t register_signal_handlers(atomic_bool *is_shutdown_requested);

#endif /* SIGNAL_HANDLER_H */
