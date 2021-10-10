#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "shared_memory.h"
#include "linked_list.h"

bool _quit;

//////////////////// Randomisation functionality:

pthread_mutex_t random_gen_mutex;

void random_init(unsigned int seed)
{
    srand(seed);
}

int random_int(int range_min, int range_max)
{
    pthread_mutex_lock(&random_gen_mutex);

    int n = rand() % (range_max - range_min +1) + range_min;

    pthread_mutex_unlock(&random_gen_mutex);
    return n;
}

char random_letter(void)
{
    return (char)random_int(65, 90);
}

char random_digit(void)
{
    return (char)random_int(30, 39);
}

//////////////////// End randomisation functionality.

//////////////////// Delay functionality:

const unsigned int time_scale;

void delay_random_ms(unsigned int range_min, unsigned int range_max)
{
    /* Generate random +ve number in given range, in milliseconds: */
    unsigned int us = random_int(range_min, range_max);
    /* Convert milliseconds to microseconds, and apply time scale: */
    us = us * 1000 * time_scale;
    usleep(us);
}

//////////////////// End delay functionality.

//////////////////// Car functionality and model:

typedef struct car_t
{
    char license_plate[6];
    uint8_t level_assigned;
    pthread_t sim_thread;
} car_t;

list_t *car_list = NULL;

void generate_license_plate(char *lplate[LICENSE_PLATE_LENGTH])
{
    /* Generate numbers: */
    for(uint8_t i = 0; i < LICENSE_PLATE_LENGTH/2; ++i)
    {
        lplate[i] = random_digit();
    }

    /* Generate letters (Capitalised ASCII): */
    for(uint8_t i = LICENSE_PLATE_LENGTH/2; i < LICENSE_PLATE_LENGTH; ++i)
    {
        lplate[i] = random_letter();
    }
}

void generate_car(car_t *new_car)
{
    /* Generate license plate: */
        /* Ensure car doesn't currently exist (no license plate duplicates): */
    char *lplate[LICENSE_PLATE_LENGTH] = &new_car->license_plate;
    generate_license_plate(lplate);
    while(llist_find(car_list, lplate) != NULL)
    {
        generate_license_plate(lplate);
    }
    
    /* Initialise other contents to defaults: */
    new_car->level_assigned = 0;
    // new_car->sim_thread = ;
}

void generate_cars_loop()
{
    while(!_quit)
    {
        /* Create node in linked list for a new car: (This will allocate memory for new car) */
        node_t *car_node = llist_push_empty(car_list->head, sizeof(car_t));

        /* Generate car: */
        car_t *new_car = (car_t *)car_node->data;
        generate_car(new_car);

        /* Generate thread to simulate car: */
        pthread_create(&new_car->sim_thread, NULL, car_sim_loop, new_car);

        /* Sleep for random time: */
        delay_random_ms(1, 100);
    }
}

int car_compare_lplate(char *lplate1[LICENSE_PLATE_LENGTH], char *lplate2[LICENSE_PLATE_LENGTH])
{
    if(strcmp(lplate1, lplate2) == 0)
    { /* Exact match. */
        return 0;
    }
    // else if()
    // { /* Before alphabetically and numerically: */
    //     /* TODO: Implement functionality to compare license plates in ascending order! */
    // }

    /* Once compare function complete, delete below return statement! */
    return -1;
}

//////////////////// End car functionality and model.

int main(void)
{
    _quit = false;

    // Simulating Car
        // Generate car

        // Car Queue up for entrance triggering LPR

        // Get information from digital sign

        // Respond to Information received from sign

        // Generate Random Time to park

        // Leave after finish parking

        // Remove Car from Simulation

    // Simulate Boom Gate
        // Get signal to open

        // Close Gate after time

    // Simulate Temperature
        // Generate Temperature
        
}