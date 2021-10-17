#ifndef  LPLATE_SENSOR_H
#define  LPLATE_SENSOR_H

#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include "shared_memory.h"

/**
 * @brief This function will wait for the given LPS to read a new license plate,
 * then return. It will wait on the LPS's condition variable after an outside
 * process updates the license plate field, then copy the value and release
 * the LPS's mutex.
 * Use this function from within the manager, in a loop function, that will handle
 * behaviour for entrances, levels or exits.
 * 
 * @param lplate_sensor Pointer to the license plate reader's structure.
 */
void lplate_sensor_read(license_plate_sensor_t *lplate_sensor, char *lplate);

#endif //LPLATE_SENSOR_H