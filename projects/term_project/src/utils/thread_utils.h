#ifndef THREAD_UTILS_H
#define THREAD_UTILS_H

#include <pthread.h>
#include <sched.h>
#include <stdint.h>

/* Temporarily elevates the calling thread to SCHED_FIFO max priority.
 * Saves the original priority into *saved_priority for restoration.
 * Used before bit-bang GPIO measurements to reduce preemption jitter. */
static inline void boost_thread_priority(int32_t *saved_priority) {
	struct sched_param sp = { 0 };
	int policy = 0;
	(void)pthread_getschedparam(pthread_self(), &policy, &sp);
	*saved_priority = (int32_t)sp.sched_priority;
	sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
	(void)pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
}

static inline void restore_thread_priority(int32_t saved_priority) {
	struct sched_param sp = { .sched_priority = (int)saved_priority };
	(void)pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
}

#endif /* THREAD_UTILS_H */
