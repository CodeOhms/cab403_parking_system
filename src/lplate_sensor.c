#include "lplate_sensor.h"

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