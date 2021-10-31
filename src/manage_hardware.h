#ifndef  MANAGE_HARDWARE_H
#define  MANAGE_HARDWARE_H

#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include "shared_memory.h"

//////////////////// Prototypes:

void lplate_sensor_read(license_plate_sensor_t *lplate_sensor, char *lplate);

void boom_gate_admit_one(boom_gate_t *boom_gate);

void boom_gate_open(boom_gate_t *boom_gate);

void boom_gate_close(boom_gate_t *boom_gate);

void info_sign_update(information_sign_t* info_sign, char display);

//////////////////// End prototypes.

//////////////////// License plate sensor functionality:

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
void lplate_sensor_read(license_plate_sensor_t *lplate_sensor, char *lplate)
{
    /* Aquire the sensor mutex: */
    pthread_mutex_lock(&lplate_sensor->lplate_sensor_mutex);

    /* Wait on signal for new car (will also unlock mutex whilst waiting): */
    pthread_cond_wait(&lplate_sensor->lplate_sensor_update_flag, &lplate_sensor->lplate_sensor_mutex);

    /* Copy the license plate into the given buffer: */
    memcpy(lplate, lplate_sensor->license_plate, sizeof(char) * LICENSE_PLATE_LENGTH);

    /* Mutex automatically reaquired after returning from wait, unlock mutex: */
    pthread_mutex_unlock(&lplate_sensor->lplate_sensor_mutex);
}

//////////////////// End license plate functionality.

//////////////////// Boom gate functionality:

void boom_gate_admit_one(boom_gate_t *boom_gate)
{
    // Acquire mutex of boomgate
    pthread_mutex_lock(&boom_gate->bgate_mutex);

    // Admit one car
    boom_gate_open(boom_gate);
    boom_gate_close(boom_gate);

    /* Unlock mutex: */
    pthread_mutex_unlock(&boom_gate->bgate_mutex);
}

void boom_gate_open(boom_gate_t *boom_gate)
{
    pthread_mutex_lock(&boom_gate->bgate_mutex);

    // Set state to rising
    boom_gate->bgate_state = R;

    // Signal condition variable 
    pthread_cond_signal(&boom_gate->bgate_update_flag);

    // Wait for boom gate to open
    pthread_cond_wait(&boom_gate->bgate_update_flag, &boom_gate->bgate_mutex);

    pthread_mutex_unlock(&boom_gate->bgate_mutex);
}

void boom_gate_close(boom_gate_t *boom_gate)
{
    pthread_mutex_lock(&boom_gate->bgate_mutex);

    /* Set state to lowering: */
    boom_gate->bgate_state = L;

    /* Signal condition variable: */
    pthread_cond_broadcast(&boom_gate->bgate_update_flag);

    /* Wait for boom gate to close: */
    pthread_cond_wait(&boom_gate->bgate_update_flag, &boom_gate->bgate_mutex);

    pthread_mutex_unlock(&boom_gate->bgate_mutex);
}

//////////////////// End boom gate functionality.

//////////////////// Information sign functionality:

/**
 * @brief Use this from the manager to update the information signs
 * at the entrances.
 */
void info_sign_update(information_sign_t* info_sign, char display)
{
    pthread_mutex_lock(&info_sign->info_sign_mutex);
    info_sign->display = display;
    pthread_mutex_unlock(&info_sign->info_sign_mutex);
    pthread_cond_signal(&info_sign->info_sign_update_flag);
}

//////////////////// End information sign functionality.

#endif //MANAGE_HARDWARE_H