#ifndef PROJECT_CONSTANTS_H
#define PROJECT_CONSTANTS_H

/* Size constants */
#define MAX_FILENAME_LENGTH (255U)
#define MAX_FILE_PATH_LENGTH (4096U)

/* Sleep timers */
#define MAIN_THREAD_SLEEP_S (1U)
#define SHUTDOWN_DELAY_S (5U)

/* Traffic Light Flashing Time */
#define LIGHT_FLASH_COUNT (4U)

/* Seconds to Nanoseconds Multiplier */
#define SEC_TO_NSEC (1000000000.0)
#define MSEC_PER_SEC (1000U)
#define NSEC_PER_MSEC (1000000L)

/* Debounce Values */
#define SAMPLE_MS (5)
#define STABLE_NEEDED (6)

/* Train Timeout Time */
#define TIMEOUT_TIME (5)

/* GPIO Types */
#define GPIO_IN ("In")
#define GPIO_OUT ("Out")

/* Filenames */
#define CONFIG_FILENAME ("/p1_config.cfg")

/* Unit conversions */
#define SEC_PER_MINUTE (60U)
#define NSEC_PER_USEC (1000U)

#endif /* PROJECT_CONSTANTS_H */
