#include "read_timer.h"

#include <time.h>

#include "project_constants.h"

static float64_t get_monotonic_time(void) {
	struct timespec ts = { 0 };
	(void)clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	return (float64_t)ts.tv_sec + ((float64_t)ts.tv_nsec / NSEC_PER_SEC_F);
}

void read_timer_init(read_timer_t *t) {
	t->last_time = 0.0;
	t->max_interval = 0.0;
	t->min_interval = 0.0;
	t->has_baseline = false;
	t->has_interval = false;
}

bool read_timer_record(read_timer_t *t, float64_t *out_elapsed) {
	float64_t now = get_monotonic_time();
	if (!t->has_baseline) {
		t->last_time = now;
		t->has_baseline = true;
		if (out_elapsed != NULL) {
			*out_elapsed = 0.0;
		}
		return false;
	}

	float64_t elapsed = now - t->last_time;
	if (!t->has_interval) {
		t->max_interval = elapsed;
		t->min_interval = elapsed;
		t->has_interval = true;
	} else {
		if (elapsed > t->max_interval) {
			t->max_interval = elapsed;
		}
		if (elapsed < t->min_interval) {
			t->min_interval = elapsed;
		}
	}

	t->last_time = now;
	if (out_elapsed != NULL) {
		*out_elapsed = elapsed;
	}
	return true;
}

float64_t read_timer_jitter(const read_timer_t *t) {
	if (!t->has_interval) {
		return 0.0;
	}
	return t->max_interval - t->min_interval;
}
