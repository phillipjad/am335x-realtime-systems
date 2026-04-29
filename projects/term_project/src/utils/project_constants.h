#ifndef PROJECT_CONSTANTS_H
#define PROJECT_CONSTANTS_H

/* Size constants */
#define MAX_FILENAME_LENGTH (255U)
#define MAX_FILE_PATH_LENGTH (4096U)
#define USER_INPUT_MAX_LEN (1024U)

/* Warning light active duration */
#define WARNING_LIGHT_ACTIVE_DURATION_MS (250L)

/* Seconds to Nanoseconds Multiplier */
#define NSEC_PER_SEC_F (1000000000.0)
#define NSEC_PER_SEC (1000000000UL)
#define MSEC_PER_SEC (1000U)
#define NSEC_PER_MSEC (1000000L)

/* Various time constants */
#define QUARTER_SECOND_AS_NSEC (250000000L)
#define HALF_SECOND_AS_NSEC (500000000L)
#define LCD_ENABLE_PULSE_NS (1000000L)

/* Debounce Values */
#define SAMPLE_MS (5)
#define STABLE_NEEDED (6)

/* Timeout Times */
#define SERVO_TIMEOUT_MS_TIME_F (200U)

/* Filenames */
#define CONFIG_FILENAME ("/tp_config.cfg")

/* Unit conversions */
#define NSEC_PER_USEC (1000U)

/* DHT-22 protocol timing */
#define DHT22_BIT_THRESHOLD_USEC (40U)    /* HIGH > 40us = bit 1, <= 40us = bit 0 */
#define DHT22_PULL_LOW_TIMEOUT_USEC (40U) /* DHT22 should pull the voltage low between 20-40 us */
#define DHT22_ACK_TIMEOUT_USEC (200U)     /* max wait for each ACK/bit phase transition */
#define DHT22_BIT_TIMEOUT_USEC (150U)
#define DHT22_DATA_BITS (40U)
#define DHT22_DATA_BYTES (5U)

/* Temp Buffer (+or- range) */
#define TEMP_BUFFER (0.5)
#define MIN_TEMP (60.0)
#define MAX_TEMP (100.0)

/* Logger Constants */
#define LOG_PARENT_DIR ("/opt/smart_vent")
#define LOG_BASE_DIR ("/opt/smart_vent/logs")
#define MAX_LOG_LEN (1000U)
#define MAX_LINENO_LEN (10U)
#define MAX_LOG_QUEUE_CAPACITY (1000U)

/* Shared sensor constants */
#define SENSOR_FAIL_THRESHOLD (15U)

#endif /* PROJECT_CONSTANTS_H */
