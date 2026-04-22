#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include "project_types.h"

static inline void increment_heartbeat(global_values_t *shared_info, thread_index_e thread_idx) {
	++shared_info->heartbeats[thread_idx];
}

#endif /* HEARTBEAT_H */
