#ifndef READ_TIMER_H
#define READ_TIMER_H

#include <stdbool.h>

#include "project_types.h"

typedef struct {
	float64_t last_time;    /**< Timestamp (s) of the most recent record */
	float64_t max_interval; /**< All-time maximum interval between consecutive records */
	float64_t min_interval; /**< All-time minimum interval between consecutive records */
	bool has_baseline;      /**< True after the first record() call */
	bool has_interval;      /**< True after the first interval is computed (2+ records) */
} read_timer_t;

/**
 * @brief Initialises a read_timer_t to a clean state.
 *        Zero-initialised (static) timers are also valid without calling this.
 */
void read_timer_init(read_timer_t *t);

/**
 * @brief Records the current monotonic timestamp and computes the elapsed time
 *        since the previous record.
 *
 * @param[in,out] t The timer to update.
 * @param[out] out_elapsed Set to elapsed seconds since last record (0.0 on first call).
 * @return true if a valid interval was computed (not the first call).
 * @return false on the first call — no prior timestamp exists yet.
 */
bool read_timer_record(read_timer_t *t, float64_t *out_elapsed);

/**
 * @brief Returns the jitter (max_interval - min_interval) observed so far.
 * Returns 0.0 if fewer than two records have been made.
 */
float64_t read_timer_jitter(const read_timer_t *t);

#endif /* READ_TIMER_H */
