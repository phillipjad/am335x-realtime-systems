#include "temperature_sensor.h"

/* Local project includes after system libraries */
#include "gpio_control.h"
#include "heartbeat.h"
#include "logger.h"
#include "project_constants.h"
#include "project_types.h"

static global_values_t *shared_info = NULL;

typedef struct {
	float64_t temp_c;
	float64_t humidity_rh;
} temp_readings_t;

/**
 * @brief Returns true if 'now' is at or past 'deadline'
 *
 * @param[in] now The current time
 * @param[in] deadline The deadline to check against
 * @return bool True if we have passed \p deadline; false otherwisw
 */
static bool past_deadline(const struct timespec *now, const struct timespec *deadline) {
	return (now->tv_sec > deadline->tv_sec) || ((now->tv_sec == deadline->tv_sec) && (now->tv_nsec >= deadline->tv_nsec));
}

/**
 * @brief Builds a deadline timespec TIMEOUT_USEC microseconds from now
 *
 * @param[out] out Timespec to write the created deadline to
 * @param timeout_usec The timeout to create the deeadline from
 */
static void make_deadline(struct timespec *out, uint32_t timeout_usec) {
	clock_gettime(CLOCK_MONOTONIC_RAW, out);
	out->tv_nsec += (long)((long)timeout_usec * (long)NSEC_PER_USEC);
	if (out->tv_nsec >= 1000000000L) {
		out->tv_sec += 1L;
		out->tv_nsec -= 1000000000L;
	}
}

/**
 * @brief Resets the data pin to idle state (output HIGH) after any read attempt.
 * Must be called on every return path from read_dht_22.
 *
 * @param pin The pin to reset
 */
static void reset_pin(uint8_t pin) {
	gpio_set_direction_out(pin);
	gpio_set(pin, GPIO_HIGH);
}

/**
 * @brief Implementation based on https://cdn.sparkfun.com/assets/f/7/d/9/c/DHT22.pdf for signalling
 * and https://www.waveshare.com/wiki/DHT22_Temperature-Humidity_Sensor for data parsing
 *
 * @param[out] out The struct to store the output from this temp sensor reading
 * @return int32_t Unix-like error code. 0 means success < 0 means error
 */
static int32_t read_dht_22(temp_readings_t *out) {
	static const struct timespec start_low_duration = { .tv_sec = 0L, .tv_nsec = 10L * NSEC_PER_MSEC };
	uint8_t data_pin = shared_info->config.gpio_layout.temp_sensor;

	/* Start signal: pull LOW >= 1ms, release HIGH, switch to input */
	gpio_set(data_pin, GPIO_LOW);
	(void)nanosleep(&start_low_duration, NULL);
	gpio_set(data_pin, GPIO_HIGH);
	gpio_set_direction(data_pin, GPIO_IN);

	/* Wait for sensor ACK LOW (sensor pulls line LOW within 40us) */
	struct timespec deadline = { 0 };
	struct timespec now = { 0 };
	make_deadline(&deadline, DHT22_PULL_LOW_TIMEOUT_USEC);
	while (gpio_read(data_pin)) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);
		if (past_deadline(&now, &deadline)) {
			reset_pin(data_pin);
			return STATUS_FAIL;
		}
	}

	/* ACK: wait for sensor to drive HIGH (~80us) */
	make_deadline(&deadline, DHT22_ACK_TIMEOUT_USEC);
	while (!gpio_read(data_pin)) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);
		if (past_deadline(&now, &deadline)) {
			reset_pin(data_pin);
			return STATUS_FAIL;
		}
	}

	/* ACK: wait for ACK HIGH to end; data begins on next LOW */
	make_deadline(&deadline, DHT22_ACK_TIMEOUT_USEC);
	while (gpio_read(data_pin)) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);
		if (past_deadline(&now, &deadline)) {
			reset_pin(data_pin);
			return STATUS_FAIL;
		}
	}

	/* Read 40 data bits */
	uint8_t raw_bytes[DHT22_DATA_BYTES] = { 0 };

	for (uint8_t bit_idx = 0U; bit_idx < DHT22_DATA_BITS; ++bit_idx) {
		/* Each bit begins with a 50us LOW separator; wait for it to end */
		make_deadline(&deadline, DHT22_ACK_TIMEOUT_USEC);
		while (!gpio_read(data_pin)) {
			clock_gettime(CLOCK_MONOTONIC_RAW, &now);
			if (past_deadline(&now, &deadline)) {
				reset_pin(data_pin);
				return STATUS_FAIL;
			}
		}

		/* Measure HIGH pulse width: 26-28us = bit 0, 70us = bit 1 */
		struct timespec high_start = { 0 };
		struct timespec high_end;
		clock_gettime(CLOCK_MONOTONIC_RAW, &high_start);

		struct timespec bit_deadline = high_start;
		make_deadline(&bit_deadline, DHT22_BIT_TIMEOUT_USEC);

		while (gpio_read(data_pin)) {
			clock_gettime(CLOCK_MONOTONIC_RAW, &high_end);
			if (past_deadline(&high_end, &bit_deadline)) {
				reset_pin(data_pin);
				return STATUS_FAIL;
			}
		}
		clock_gettime(CLOCK_MONOTONIC_RAW, &high_end);

		int64_t high_ns = ((int64_t)(high_end.tv_sec - high_start.tv_sec) * NSEC_PER_SEC) +
		    ((int64_t)(high_end.tv_nsec - high_start.tv_nsec));
		uint8_t bit_val = (high_ns > (int64_t)((int64_t)DHT22_BIT_THRESHOLD_USEC * (int64_t)NSEC_PER_USEC)) ? 1U : 0U;

		uint8_t byte_idx = bit_idx / 8U;
		raw_bytes[byte_idx] = (uint8_t)((raw_bytes[byte_idx] << 1U) | bit_val);
	}

	/* Checksum validation */
	uint8_t expected = (uint8_t)((raw_bytes[0] + raw_bytes[1] + raw_bytes[2] + raw_bytes[3]) & 0xFFU);
	if (expected != raw_bytes[4]) {
		LOG(TEMP_SENSOR, "DHT22 checksum mismatch: expected 0x%02X got 0x%02X", expected, raw_bytes[4]);
		reset_pin(data_pin);
		return STATUS_FAIL;
	}

	/* Convert raw bytes to physical values */
	uint16_t raw_hum = ((uint16_t)raw_bytes[0] << 8U) | (uint16_t)raw_bytes[1];
	/* We intentionally ignore the negative MSB here */
	uint16_t raw_temp = ((uint16_t)(raw_bytes[2] & 0x7FU) << 8U) | (uint16_t)raw_bytes[3];
	out->humidity_rh = (float64_t)raw_hum / 10.0;
	out->temp_c = (float64_t)raw_temp / 10.0;
	/* Manually check for negative temp reading and handle */
	if ((raw_bytes[2] & 0x80U) != 0U) {
		out->temp_c = -(out->temp_c);
	}

	reset_pin(data_pin);
	return STATUS_SUCCESS;
}

/**
 * @brief Generic temp sensor wrapper function
 *
 * @param[out] out Struct to read values from the temp sensor into
 * @return int32_t Unix-like error code. 0 means success < 0 means error
 */
static int32_t read_temp_sensor(temp_readings_t *out) {
	return read_dht_22(out);
}

void *temperature_sensor_thread_entry(void *arg) {
	LOG(TEMP_SENSOR, "Starting temperature sensor thread");
	shared_info = (global_values_t *)arg;
	/* Sleep 3.5 seconds between reads to not overload sensor */
	const struct timespec thread_sleep = { .tv_sec = 3, .tv_nsec = 500 * NSEC_PER_MSEC };

	uint8_t failed_sensor_reads = 0U;
	while (!atomic_load(&shared_info->is_shutdown_requested)) {
		temp_readings_t readings = { 0 };
		int32_t result = read_temp_sensor(&readings);
		if (result != STATUS_SUCCESS) {
			++failed_sensor_reads;
		} else {
			pthread_mutex_lock(&shared_info->mutex);
			shared_info->current_temp = readings.temp_c;
			shared_info->current_humidity_rh = readings.humidity_rh;
			pthread_mutex_unlock(&shared_info->mutex);
			LOG(TEMP_SENSOR, "Temp: %.3lf C  Humidity: %.1f %%RH", readings.temp_c, readings.humidity_rh);
		}

		if (failed_sensor_reads > SENSOR_FAIL_THRESHOLD) {
			pthread_mutex_lock(&shared_info->mutex);
			set_error(&shared_info->thread_errors[TEMP_SENSOR], "DHT22 failed %u consecutive sensor readings", SENSOR_FAIL_THRESHOLD);
			pthread_mutex_unlock(&shared_info->mutex);
		} else {
			if (result == STATUS_SUCCESS) {
				pthread_mutex_lock(&shared_info->mutex);
				clear_error(&shared_info->thread_errors[TEMP_SENSOR]);
				pthread_mutex_unlock(&shared_info->mutex);
			}
		}

		increment_heartbeat(shared_info, TEMP_SENSOR);
		(void)nanosleep(&thread_sleep, NULL);
	}

	LOG(TEMP_SENSOR, "Shutting down temperature sensor thread");
	return NULL;
}
