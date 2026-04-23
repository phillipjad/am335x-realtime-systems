#ifndef PROJECT_CONSTANTS_H
#define PROJECT_CONSTANTS_H

/* Size constants */
#define MAX_FILENAME_LENGTH (255U)
#define MAX_FILE_PATH_LENGTH (4096U)
#define USER_INPUT_MAX_LEN (1024U)

/* Warning light active duration */
#define WARNING_LIGHT_ACTIVE_DURATION_MS (250L)

/* Seconds to Nanoseconds Multiplier */
#define SEC_TO_NSEC (1000000000.0)
#define MSEC_PER_SEC (1000U)
#define NSEC_PER_MSEC (1000000L)

/* Debounce Values */
#define SAMPLE_MS (5)
#define STABLE_NEEDED (6)

/* Timeout Times */
#define SERVO_TIMEOUT_MS_TIME_F (200U)

/* Filenames */
#define CONFIG_FILENAME ("/tp_config.cfg")

/* Unit conversions */
#define NSEC_PER_USEC (1000U)

/* System states */
#define FAIL ("Fail")
#define AUTO ("Auto")

#endif /* PROJECT_CONSTANTS_H */
