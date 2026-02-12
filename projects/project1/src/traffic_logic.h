#ifndef TRAFFIC_LOGIC_H
#define TRAFFIC_LOGIC_H

/**
 * @brief Gracefully shuts down the system
 */
void handle_shutdown(void);

/**
 * @brief Runs the applications' traffic signal logic
 */
void run_traffic_signal(void);

#endif /* TRAFFIC_LOGIC_H */
