#include <string.h>
#include <unistd.h>

#include "heartbeat.h"
#include "log_handler.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

static global_values_t *shared_info = NULL;
static FILE *log_files[NUM_THREADS];

/*--------------------------------------
 * Static Function: write_log_message
 *--------------------------------------*/
static void write_log_message(const log_queue_message_t *msg) {
	char output_buffer[MAX_LOG_LEN + 1U] = { 0 };
	struct timespec curr_time = { 0 };
	(void)clock_gettime(CLOCK_REALTIME, &curr_time);
	int64_t microseconds = curr_time.tv_nsec / NSEC_PER_USEC;
	int32_t used = snprintf(output_buffer, MAX_LOG_LEN, "%" PRIdMAX ".%.6" PRId64, (intmax_t)curr_time.tv_sec, microseconds);

	const char *basename = strrchr(msg->filename, '/');
	basename = (basename != NULL) ? (basename + 1) : msg->filename;
	used += snprintf(&output_buffer[used], (MAX_LOG_LEN - (size_t)used), "[%s:%" PRIu32 "]: ", basename, msg->line_no);
	used += snprintf(&output_buffer[used], (MAX_LOG_LEN - (size_t)used), "%s", msg->message);
	if (msg->include_newline) {
		(void)snprintf(&output_buffer[used], (MAX_LOG_LEN - (size_t)used), "\n");
	}

	printf("%s", output_buffer);
	(void)fflush(stdout);

	if (msg->thread_id < NUM_THREADS && log_files[msg->thread_id] != NULL) {
		fprintf(log_files[msg->thread_id], "%s", output_buffer);
		(void)fflush(log_files[msg->thread_id]);
	}
}

/*--------------------------------------
 * Function: log_handler_thread_entry
 *--------------------------------------*/
void *log_handler_thread_entry(void *arg) {
	shared_info = (global_values_t *)arg;

	const char *base = get_log_base_path();
	for (uint32_t ii = 0U; ii < NUM_THREADS; ++ii) {
		char path[MAX_FILE_PATH_LENGTH + 1U] = { 0 };
		(void)snprintf(path, MAX_FILE_PATH_LENGTH, "%s%s.log", base, THREAD_NAMES[ii]);
		log_files[ii] = fopen(path, "a");
		if (log_files[ii] == NULL) {
			LOG_AND_EXIT("Failed to open log file: %s", path);
		}
	}

	LOG(LOG_HANDLER, "Starting log handler thread!");

	log_queue_t *queue = &shared_info->logger;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		struct timespec timeout = { 0 };
		(void)clock_gettime(CLOCK_REALTIME, &timeout);
		timeout.tv_sec += 1;

		pthread_mutex_lock(&queue->log_mu);
		while (queue->size == 0U && !atomic_load(&shared_info->is_shutdown_requested)) {
			(void)pthread_cond_timedwait(&queue->log_cv, &queue->log_mu, &timeout);
			/* Rebuild timeout each pass so timedwait doesn't return immediately on repeat */
			(void)clock_gettime(CLOCK_REALTIME, &timeout);
			timeout.tv_sec += 1;
			/* Heartbeat on every wakeup so the monitor sees progress even with an idle queue */
			increment_heartbeat(shared_info, LOG_HANDLER);
		}
		while (queue->size > 0U) {
			log_queue_message_t msg = queue->queue[queue->head];
			queue->head = (queue->head + 1U) % MAX_LOG_QUEUE_CAPACITY;
			--queue->size;
			pthread_mutex_unlock(&queue->log_mu);
			write_log_message(&msg);
			pthread_mutex_lock(&queue->log_mu);
		}
		pthread_mutex_unlock(&queue->log_mu);
	}

	/* Final log before files are closed */
	LOG(LOG_HANDLER, "Shutting down log handler thread");

	/* Drain any messages enqueued during or after shutdown (including the message above) */
	pthread_mutex_lock(&queue->log_mu);
	while (queue->size > 0U) {
		log_queue_message_t msg = queue->queue[queue->head];
		queue->head = (queue->head + 1U) % MAX_LOG_QUEUE_CAPACITY;
		queue->size--;
		pthread_mutex_unlock(&queue->log_mu);
		write_log_message(&msg);
		pthread_mutex_lock(&queue->log_mu);
	}
	pthread_mutex_unlock(&queue->log_mu);

	for (uint32_t ii = 0U; ii < NUM_THREADS; ++ii) {
		(void)fclose(log_files[ii]);
	}

	return NULL;
}
