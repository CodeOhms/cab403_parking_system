#ifndef  UTILS_H
#define  UTILS_H

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include "htab.h"
#include "shared_memory.h"

//////////////////// Randomisation functionality:

void random_init(pthread_mutex_t *mutex, unsigned int seed)
{
    pthread_mutex_init(mutex, NULL);
    srand(seed);
}

void random_close(pthread_mutex_t *mutex)
{
    pthread_mutex_destroy(mutex);
}

int random_int(pthread_mutex_t *mutex, int range_min, int range_max)
{
    pthread_mutex_lock(mutex);

    int n = rand() % (range_max - range_min +1) + range_min;

    pthread_mutex_unlock(mutex);
    return n;
}

char random_letter(pthread_mutex_t *mutex)
{
    return (char)random_int(mutex, 'A', 'Z');
}

char random_digit(pthread_mutex_t *mutex)
{
    return (char)random_int(mutex, '0', '9');
}

//////////////////// End randomisation functionality.

//////////////////// Delay functionality:

void delay_ms(unsigned int t, unsigned int time_scale)
{
    unsigned int us = t * 1000 * time_scale;
    usleep(us);
}

void delay_random_ms(pthread_mutex_t *mutex, unsigned int range_min, unsigned int range_max, unsigned int time_scale)
{
    /* Generate random +ve number in given range, in milliseconds: */
    unsigned int us = random_int(mutex, range_min, range_max);
    /* Convert milliseconds to microseconds, and apply time scale: */
    us = us * 1000 * time_scale;
    usleep(us);
}

//////////////////// End delay functionality.

//////////////////// File I/O functionality:

// Setup License plate for reading
void lp_list ( htab_t *htable, char auth_lplates[][LICENSE_PLATE_LENGTH + 1] ) {

    FILE *plate = fopen("plates.txt", "r");

    const unsigned MAX_LENGTH = 256;
    char buffer[LICENSE_PLATE_LENGTH + 1];

    /* Each line contains one license plate string: */
    size_t line;

    // Assign authorised license plates to Hash Table
    while (fgets(buffer, MAX_LENGTH, plate)) {
        strcpy(auth_lplates[line], buffer);
        auth_lplates[line][LICENSE_PLATE_LENGTH] = '\0';
        htab_add(htable, auth_lplates[line], line+1);
        line++;
    }

    // close the file
    fclose(plate);
}

//////////////////// End file I/O functionality.

#endif //UTILS_H