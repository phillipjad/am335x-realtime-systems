#ifndef LAB_B_H
#define LAB_B_H

#define _POSIX_C_SOURCE 202405L

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#define NSEC_PER_SEC (1000000000LL)
#define NSEC_PER_MSEC (1000000LL)
#define NSEC_PER_USEC (1000LL)

/* Thread periods */
#define T1_PERIOD_NS (10LL * NSEC_PER_MSEC)  /* 10 ms  */
#define T2_PERIOD_NS (20LL * NSEC_PER_MSEC)  /* 20 ms  */
#define T3_PERIOD_NS (100LL * NSEC_PER_MSEC) /* 100 ms */

/* Target work duration per activation (~10% of each period) */
#define T1_WORK_NS (1LL * NSEC_PER_MSEC)  /* 1 ms  in 10 ms period  */
#define T2_WORK_NS (2LL * NSEC_PER_MSEC)  /* 2 ms  in 20 ms period  */
#define T3_WORK_NS (10LL * NSEC_PER_MSEC) /* 10 ms in 100 ms period */

typedef double float64_t;

/* RMS FIFO priorities (shorter period → higher priority) */
#define T1_FIFO_PRIO (97)
#define T2_FIFO_PRIO (96)
#define T3_FIFO_PRIO (95)
#define MAIN_FIFO_PRIO (94) /* main thread, below all workers */

#define THREAD_COUNT (3U)
#define MAX_SAMPLES_PER_THREAD (2048U) /* covers ~20s at 10ms period; ~80KB on stack */
#define DEFAULT_DURATION_SEC (10U)
#define MAX_CSV_PATH_LEN (4096U)

#define STATUS_SUCCESS (0)
#define STATUS_FAIL (-1)

#define CSV_HEADER "thread_id,iteration,period_ns,scheduled_ns,actual_ns,jitter_ns,work_ns,deadline_missed\n"

/* One activation measurement row */
typedef struct {
	uint32_t thread_id;
	uint64_t iteration;
	int64_t period_ns;        /* thread period — makes CSV self-describing */
	int64_t scheduled_ns;     /* intended release time */
	int64_t actual_ns;        /* actual start time */
	int64_t jitter_ns;        /* lateness = actual - scheduled */
	int64_t work_ns;          /* measured work duration for this activation */
	uint32_t deadline_missed; /* 1 if job finished after next release, else 0 */
} sample_t;

/* Immutable per-thread configuration, passed by value into thread_args_t */
typedef struct {
	uint32_t thread_id;
	int64_t period_ns;
	int64_t work_duration_ns; /* target CPU work per activation */
	int32_t fifo_priority;    /* ignored when sched_policy == SCHED_OTHER */
	int32_t sched_policy;     /* SCHED_FIFO or SCHED_OTHER */
	uint64_t duration_sec;
	atomic_bool *shutdown; /* shared stop flag, set by signal handler */
} thread_config_t;

/* Per-thread accumulated statistics, written back to main after thread exits */
typedef struct {
	uint64_t sample_count;
	uint64_t missed_deadlines;
	int64_t jitter_min_ns;
	int64_t jitter_max_ns;
	int64_t jitter_sum_ns;
	float64_t jitter_avg_ns;
	int64_t work_min_ns;
	int64_t work_max_ns;
	int64_t work_sum_ns;
	float64_t work_avg_ns;
	sample_t samples[MAX_SAMPLES_PER_THREAD]; /* stack-allocated, no I/O in hot loop */
} thread_stats_t;

/* Single struct passed to pthread_create */
typedef struct {
	thread_config_t config;    /* by value — safe, no pointer aliasing */
	thread_stats_t *stats_out; /* pointer to pre-allocated slot in main's array */
} thread_args_t;

/* Parsed CLI options */
typedef struct {
	int32_t sched_policy;
	uint64_t duration_sec;
	char csv_path[MAX_CSV_PATH_LEN + 1U];
} app_config_t;

void *periodic_thread_entry(void *arg);

#endif /* LAB_B_H */
