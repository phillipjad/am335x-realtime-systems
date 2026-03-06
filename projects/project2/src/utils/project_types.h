#ifndef PROJECT_TYPES_H
#define PROJECT_TYPES_H

#include "pthread.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STATUS_SUCCESS (0)
#define STATUS_FAIL (-1)

typedef double float64_t;

typedef struct {
	pthread_mutex_t mutex;
	atomic_bool is_shutdown_requested;
} global_values_t;

#endif /* PROJECT_TYPES_H */
