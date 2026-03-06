#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <stdint.h>

/**
 * @brief Registers SIGINT and SIGTERM signal handlers
 *
 * @return int32_t Returns STATUS_SUCCESS on success and STATUS_FAIL on failure
 */
int32_t register_signal_handlers(void);

/**
 * @brief Check if shutdown has been requested via signal
 *
 * @return int32_t Returns non-zero if shutdown requested, 0 otherwise
 */
int32_t is_shutdown_requested(void);

#endif /* SIGNAL_HANDLER_H */
