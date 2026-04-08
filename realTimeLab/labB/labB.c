#include <unistd.h>
#define _POSIX_C_SOURCE 202405L

#include "labB.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static atomic_bool g_shutdown = false;

static void timespec_add_ns(struct timespec *ts, int64_t ns) {
	ts->tv_nsec += ns;
	ts->tv_sec += ts->tv_nsec / NSEC_PER_SEC;
	ts->tv_nsec = ts->tv_nsec % NSEC_PER_SEC;
}

static int64_t timespec_to_ns(const struct timespec *ts) {
	return (int64_t)ts->tv_sec * NSEC_PER_SEC + (int64_t)ts->tv_nsec;
}

/* Returns (a - b) in nanoseconds, signed */
static int64_t timespec_diff_ns(const struct timespec *a, const struct timespec *b) {
	return timespec_to_ns(a) - timespec_to_ns(b);
}

/*
 * Time-bounded busy work: spin calling clock_gettime until work_duration_ns
 * of wall time has elapsed. Returns the actual measured work duration in nanoseconds.
 */
static int64_t do_work(int64_t work_duration_ns) {
	struct timespec start = { 0 };
	struct timespec now = { 0 };
	struct timespec deadline = { 0 };
	clock_gettime(CLOCK_MONOTONIC, &start);
	deadline = start;
	timespec_add_ns(&deadline, work_duration_ns);
	do {
		clock_gettime(CLOCK_MONOTONIC, &now);
	} while (timespec_diff_ns(&now, &deadline) < 0);
	return timespec_to_ns(&now) - timespec_to_ns(&start);
}

static void signal_handler(int32_t signum) {
	(void)signum;
	atomic_store(&g_shutdown, true);
}

void *periodic_thread_entry(void *arg) {
	thread_args_t *ta = (thread_args_t *)arg;
	thread_config_t cfg = ta->config; /* local copy */
	thread_stats_t stats = {
		.jitter_min_ns = INT64_MAX,
		.jitter_max_ns = INT64_MIN,
		.work_min_ns = INT64_MAX,
		.work_max_ns = INT64_MIN,
	};

	/* Establish the first absolute release time: now + 1 period */
	struct timespec next_wakeup = { 0 };
	clock_gettime(CLOCK_MONOTONIC, &next_wakeup);
	timespec_add_ns(&next_wakeup, cfg.period_ns);

	/* Compute absolute stop time */
	struct timespec stop_time = { 0 };
	clock_gettime(CLOCK_MONOTONIC, &stop_time);
	timespec_add_ns(&stop_time, (int64_t)(cfg.duration_sec) * NSEC_PER_SEC);

	while (!atomic_load(cfg.shutdown) && stats.sample_count < MAX_SAMPLES_PER_THREAD) {

		int64_t scheduled_ns = timespec_to_ns(&next_wakeup);

		/*
		 * Sleep until the absolute release time
		 * TIMER_ABSTIME ensures the release grid is fixed at t0 + k*period;
		 * jitter in one period cannot accumulate into future periods.
		 */
		int32_t ret = 0;
		do {
			ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_wakeup, NULL);
		} while (ret == EINTR && !atomic_load(cfg.shutdown));

		if (atomic_load(cfg.shutdown)) {
			break;
		}

		/* Record actual start time on the same clock used for sleep */
		struct timespec actual_ts = { 0 };
		clock_gettime(CLOCK_MONOTONIC, &actual_ts);
		int64_t actual_ns = timespec_to_ns(&actual_ts);
		int64_t jitter_ns = actual_ns - scheduled_ns; /* lateness; >= 0 with ABSTIME */

		/* Do bounded work and measure its actual duration */
		int64_t work_ns = do_work(cfg.work_duration_ns);

		/*
		 * Missed deadline: the job finishes after its next release time.
		 * This covers both a late start (jitter > period) and a work overrun.
		 */
		uint32_t deadline_missed = (actual_ns + work_ns > scheduled_ns + cfg.period_ns) ? 1U : 0U;
		stats.missed_deadlines += deadline_missed;

		/* Update lateness stats */
		if (jitter_ns < stats.jitter_min_ns) {
			stats.jitter_min_ns = jitter_ns;
		}
		if (jitter_ns > stats.jitter_max_ns) {
			stats.jitter_max_ns = jitter_ns;
		}
		stats.jitter_sum_ns += jitter_ns;

		/* Update work duration stats */
		if (work_ns < stats.work_min_ns) {
			stats.work_min_ns = work_ns;
		}
		if (work_ns > stats.work_max_ns) {
			stats.work_max_ns = work_ns;
		}
		stats.work_sum_ns += work_ns;

		/* Store sample — no I/O in the hot loop */
		stats.samples[stats.sample_count] = (sample_t){
			.thread_id = cfg.thread_id,
			.iteration = stats.sample_count,
			.period_ns = cfg.period_ns,
			.scheduled_ns = scheduled_ns,
			.actual_ns = actual_ns,
			.jitter_ns = jitter_ns,
			.work_ns = work_ns,
			.deadline_missed = deadline_missed,
		};
		stats.sample_count++;

		/* Check duration */
		if (timespec_diff_ns(&actual_ts, &stop_time) >= 0) {
			break;
		}

		/*
		 * Advance from the SCHEDULED time, not actual_ts.
		 * If the job overran, next_wakeup is already in the past and
		 * clock_nanosleep returns immediately, self-correcting to the grid.
		 */
		timespec_add_ns(&next_wakeup, cfg.period_ns);
	}

	/* Finalize averages */
	if (stats.sample_count > 0U) {
		stats.jitter_avg_ns = (float64_t)stats.jitter_sum_ns / (float64_t)stats.sample_count;
		stats.work_avg_ns = (float64_t)stats.work_sum_ns / (float64_t)stats.sample_count;
	}

	/* Write back to main's pre-allocated array */
	*ta->stats_out = stats;
	return NULL;
}

static void write_csv(FILE *f, const thread_stats_t *stats, uint32_t nthreads) {
	fprintf(f, CSV_HEADER);
	for (uint32_t t = 0U; t < nthreads; ++t) {
		for (uint64_t ii = 0U; ii < stats[t].sample_count; ++ii) {
			const sample_t *s = &stats[t].samples[ii];
			fprintf(f, "%" PRIu32 ",%" PRIu64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRIu32 "\n",
			    s->thread_id, s->iteration + 1U, s->period_ns, s->scheduled_ns, s->actual_ns, s->jitter_ns, s->work_ns,
			    s->deadline_missed);
		}
	}
}

static void print_summary(const thread_stats_t stats[THREAD_COUNT], const thread_config_t cfgs[THREAD_COUNT], int32_t sched_policy) {
	const char *policy_name = (sched_policy == SCHED_FIFO) ? "SCHED_FIFO" : "SCHED_OTHER";
	printf("Policy : %s\n\n", policy_name);
	printf("%s %s %s %s %s %s %s %s\n", "Thread", "Period(ms)", "Samples", "Missed", "Jitter_Max(us)", "Jitter_Avg(us)",
	    "Work_Avg(us)", "Work_Max(us)");

	for (uint32_t ii = 0U; ii < THREAD_COUNT; ++ii) {
		int64_t period_ms = cfgs[ii].period_ns / NSEC_PER_MSEC;
		printf("T%" PRIu32 " %" PRId64 " %" PRIu64 " %" PRIu64 " %.1f %.1f %.1f %.1f\n", cfgs[ii].thread_id, period_ms,
		    stats[ii].sample_count, stats[ii].missed_deadlines,
		    (stats[ii].sample_count > 0U) ? (float64_t)stats[ii].jitter_max_ns / 1000.0 : 0.0, stats[ii].jitter_avg_ns / 1000.0,
		    stats[ii].work_avg_ns / 1000.0, (stats[ii].sample_count > 0U) ? (float64_t)stats[ii].work_max_ns / 1000.0 : 0.0);
	}
}

static void print_usage(const char *prog) {
	fprintf(stderr,
	    "Usage: %s [--sched-other | --sched-fifo] [--duration <sec>] [--output <path>]\n"
	    "\n"
	    "  --sched-other        Use SCHED_OTHER / CFS (default, no root required)\n"
	    "  --sched-fifo         Use SCHED_FIFO with RMS priorities (requires root)\n"
	    "  --duration <sec>     Run duration in seconds (default: %u)\n"
	    "  --output <path>      Path for raw CSV output (optional)\n"
	    "\n"
	    "Examples:\n"
	    "  %s --sched-fifo --duration 30 --output /tmp/fifo_idle.csv\n"
	    "  %s --sched-other --duration 10\n",
	    prog, DEFAULT_DURATION_SEC, prog, prog);
}

static int32_t parse_args(int32_t argc, char *argv[], app_config_t *out) {
	out->sched_policy = SCHED_OTHER;
	out->duration_sec = DEFAULT_DURATION_SEC;
	out->csv_path[0] = '\0';

	for (int32_t ii = 1; ii < argc; ++ii) {
		if (strcmp(argv[ii], "--sched-other") == 0) {
			out->sched_policy = SCHED_OTHER;
		} else if (strcmp(argv[ii], "--sched-fifo") == 0) {
			out->sched_policy = SCHED_FIFO;
		} else if (strcmp(argv[ii], "--help") == 0 || strcmp(argv[ii], "-h") == 0) {
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
		} else if (strcmp(argv[ii], "--duration") == 0) {
			if (ii + 1 >= argc) {
				fprintf(stderr, "error: --duration requires an argument\n");
				print_usage(argv[0]);
				return STATUS_FAIL;
			}
			++ii;
			char *end = NULL;
			uint64_t val = (uint64_t)strtoull(argv[ii], &end, 10);
			if (*end != '\0' || val == 0U) {
				fprintf(stderr, "error: invalid duration '%s'\n", argv[ii]);
				print_usage(argv[0]);
				return STATUS_FAIL;
			}
			out->duration_sec = val;
		} else if (strcmp(argv[ii], "--output") == 0) {
			if (ii + 1 >= argc) {
				fprintf(stderr, "error: --output requires an argument\n");
				print_usage(argv[0]);
				return STATUS_FAIL;
			}
			++ii;
			strncpy(out->csv_path, argv[ii], MAX_CSV_PATH_LEN);
			out->csv_path[MAX_CSV_PATH_LEN] = '\0';
		} else {
			fprintf(stderr, "error: unknown flag '%s'\n", argv[ii]);
			print_usage(argv[0]);
			return STATUS_FAIL;
		}
	}
	return STATUS_SUCCESS;
}

int main(int32_t argc, char *argv[]) {
	app_config_t app_cfg = { 0 };
	if (parse_args(argc, argv, &app_cfg) != STATUS_SUCCESS) {
		return EXIT_FAILURE;
	}

	/* SCHED_FIFO setup: elevate main thread below worker threads */
	if (app_cfg.sched_policy == SCHED_FIFO) {
		struct sched_param sp = { .sched_priority = MAIN_FIFO_PRIO };
		if (sched_setscheduler(getpid(), SCHED_FIFO, &sp) != 0) {
			if (errno == EPERM) {
				fprintf(stderr,
				    "error: SCHED_FIFO requires CAP_SYS_NICE — "
				    "run as root or: sudo setcap cap_sys_nice+ep ./labB\n");
				return EXIT_FAILURE;
			}
			fprintf(stderr, "warning: sched_setscheduler for main failed: %s\n", strerror(errno));
		}
	}

	/* Signal handlers */
	struct sigaction sa = { .sa_handler = signal_handler };
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	/* Thread configuration templates */
	static const thread_config_t THREAD_TEMPLATES[THREAD_COUNT] = {
		{ .thread_id = 1U, .period_ns = T1_PERIOD_NS, .work_duration_ns = T1_WORK_NS, .fifo_priority = T1_FIFO_PRIO },
		{ .thread_id = 2U, .period_ns = T2_PERIOD_NS, .work_duration_ns = T2_WORK_NS, .fifo_priority = T2_FIFO_PRIO },
		{ .thread_id = 3U, .period_ns = T3_PERIOD_NS, .work_duration_ns = T3_WORK_NS, .fifo_priority = T3_FIFO_PRIO },
	};

	thread_stats_t stats[THREAD_COUNT] = { 0 };
	thread_args_t args[THREAD_COUNT] = { 0 };
	pthread_t threads[THREAD_COUNT] = { 0 };

	for (uint32_t ii = 0U; ii < THREAD_COUNT; ++ii) {
		args[ii].config = THREAD_TEMPLATES[ii];
		args[ii].config.sched_policy = app_cfg.sched_policy;
		args[ii].config.duration_sec = app_cfg.duration_sec;
		args[ii].config.shutdown = &g_shutdown;
		args[ii].stats_out = &stats[ii];
	}

	/* Launch threads with scheduling policy applied before first instruction */
	for (uint32_t ii = 0U; ii < THREAD_COUNT; ++ii) {
		struct sched_param sp = {
			.sched_priority = (app_cfg.sched_policy == SCHED_FIFO) ? args[ii].config.fifo_priority : 0,
		};

		pthread_attr_t attr = { 0 };
		pthread_attr_init(&attr);
		pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
		pthread_attr_setschedpolicy(&attr, app_cfg.sched_policy);
		pthread_attr_setschedparam(&attr, &sp);

		int32_t ret = pthread_create(&threads[ii], &attr, periodic_thread_entry, &args[ii]);
		pthread_attr_destroy(&attr);

		if (ret != 0) {
			if (ret == EPERM) {
				fprintf(stderr,
				    "error: SCHED_FIFO requires CAP_SYS_NICE — "
				    "run as root or: sudo setcap cap_sys_nice+ep ./labB\n");
			} else {
				fprintf(stderr, "error: pthread_create[%u] failed: %s\n", ii, strerror(ret));
			}
			atomic_store(&g_shutdown, true);
			for (uint32_t jj = 0U; jj < ii; ++jj) {
				pthread_join(threads[jj], NULL);
			}
			return EXIT_FAILURE;
		}
	}

	/* Wait */
	for (uint32_t ii = 0U; ii < THREAD_COUNT; ++ii) {
		pthread_join(threads[ii], NULL);
	}

	/* Extract configs for summary */
	thread_config_t cfgs[THREAD_COUNT] = { 0 };
	for (uint32_t ii = 0U; ii < THREAD_COUNT; ++ii) {
		cfgs[ii] = args[ii].config;
	}

	/* Bulk-write CSV — single-threaded, no mutex needed */
	if (app_cfg.csv_path[0] != '\0') {
		FILE *f = fopen(app_cfg.csv_path, "w");
		if (f == NULL) {
			fprintf(stderr, "error: cannot open CSV path '%s': %s\n", app_cfg.csv_path, strerror(errno));
		} else {
			write_csv(f, stats, THREAD_COUNT);
			fclose(f);
			printf("CSV written to: %s\n", app_cfg.csv_path);
		}
	}

	print_summary(stats, cfgs, app_cfg.sched_policy);
	return EXIT_SUCCESS;
}
