#ifndef POTENTIOMETER_H
#define POTENTIOMETER_H

#include "project_types.h"
#include <unistd.h>

void potentiometer_init(uint8_t pin_number);

void *potentiometer_thread_entry(void *arg);

#endif /* POTENTIOMETER_H */
