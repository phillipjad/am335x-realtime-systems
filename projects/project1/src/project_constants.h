#ifndef PROJECT_CONSTANTS_H
#define PROJECT_CONSTANTS_H

/* Size constants */
#define MAX_FILENAME_LENGTH (255U)
#define MAX_FILE_PATH_LENGTH (4096U)

/* Sleep timers */
#define MAIN_THREAD_SLEEP_S (1U)
#define SHUTDOWN_DELAY_S (5U)

/* Traffic Light Flashing Time */
#define LIGHT_FLASH_TIME (4U)

/* Light Directions */
#define NORTH_SOUTH ("North/South")
#define EAST_WEST ("East/West")

/* Light Colors */
#define GREEN ("Green")
#define YELLOW ("Yellow")
#define RED ("Red")

/* Filenames */
#define CONFIG_FILENAME ("/p1_config.cfg")

/* Unit conversions */
#define SEC_PER_MINUTE (60U)

#endif /* PROJECT_CONSTANTS_H */
