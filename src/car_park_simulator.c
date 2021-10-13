#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "shared_memory.h"
#include "linked_list.h"

bool _quit;

//////////////////// Shared memory functionality:

/**
 * @brief Initialise a shared_object_t, creating a block of shared memory
 * with the designated name, and setting its storage capacity to the size of a
 * shared data block.
 *
 * PRE: n/a
 *
 * POST: shm_unlink has been invoked to delete any previous instance of the
 *       shared memory object, if it exists.
 * AND   The share name has been saved in shm->name.
 * AND   shm_open has been invoked to obtain a file descriptor connected to a
 *       newly created shared memory object with size equal to the size of a
 *       shared_data_t struct, with support for read and write operations. The
 *       file descriptor should be saved in shm->fd, regardless of the final outcome.
 * AND   ftruncate has been invoked to set the size of the shared memory object
 *       equal to the size of a shared_data_t struct.
 * AND   mmap has been invoked to obtain a pointer to the shared memory, and the
 *       result has been stored in shm->data, regardless of the final outcome.
 * AND   (this code is provided for you, don't interfere with it) The shared
 *       semaphore has been initialised to manage access to the shared buffer.
 * AND   Semaphores have been initialised in a waiting state.
 *
 * @param shm The address of a shared memory control structure which will be
 *            populated by this function.
 * @param share_name The unique string used to identify the shared memory object.
 * @returns Returns true if and only if shm->fd >= 0 and shm->data != MAP_FAILED.
 *          Even if false is returned, shm->fd should contain the value returned
 *          by shm_open, and shm->data should contain the value returned by mmap.
 */
bool create_shared_object(shared_mem_t* shm) {
    // Remove any previous instance of the shared memory object, if it exists.
    shm_unlink(shm_name);

    // Assign share name to shm->name.
    // shm->name = share_name;

    // Create the shared memory object, allowing read-write access, and saving the
    // resulting file descriptor in shm->fd. If creation failed, ensure 
    // that shm->data is NULL and return false.
    if ((shm->fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666)) < 0)
    {
        shm->data = NULL;
        return false;
    }

    // Set the capacity of the shared memory object via ftruncate. If the 
    // operation fails, ensure that shm->data is NULL and return false. 
    if(ftruncate(shm->fd, shm_size) == -1)
    {
        shm->data = NULL;
        return false;
    }

    // Otherwise, attempt to map the shared memory via mmap, and save the address
    // in shm->data. If mapping fails, return false.
    shm->data = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm->fd, 0);
    if(shm->data == MAP_FAILED)
    {
        return false;
    }

    // Do not alter the following semaphore initialisation code.
    // sem_init( &shm->data->controller_semaphore, 1, 0 );
    // sem_init( &shm->data->worker_semaphore, 1, 0 );

    // If we reach this point we should return true.
    return true;
}

/**
 * @brief Destroys the shared memory object managed by a shared memory
 * control block.
 *
 * PRE: create_shared_object( shm, shm->name ) has previously been
 *      successfully invoked.
 *
 * POST: munmap has been invoked to remove the mapped memory from the address
 *       space
 * AND   shm_unlink has been invoked to remove the object
 * AND   shm->fd == -1
 * AND   shm->data == NULL.
 *
 * \param shm The address of a shared memory control block.
 */
void destroy_shared_object( shared_mem_t* shm ) {
    // Remove the shared memory object.
    munmap(shm, shm_size);
    shm_unlink(shm_name);
    shm->fd = -1;
    shm->data = NULL;
}

//////////////////// End shared memory functionality.

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

void generate_license_plate(char *lplate[license_plate_lenth])
{
    /* Generate numbers: */
    for(uint8_t i = 0; i < license_plate_lenth/2; ++i)
    {
        lplate[i] = random_digit();
    }

    /* Generate letters (Capitalised ASCII): */
    for(uint8_t i = license_plate_lenth/2; i < license_plate_lenth; ++i)
    {
        lplate[i] = random_letter();
    }
}

void generate_car(car_t *new_car)
{
    /* Generate license plate: */
        /* Ensure car doesn't currently exist (no license plate duplicates): */
    char *lplate[license_plate_lenth] = &new_car->license_plate;
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
        node_t *car_node = llist_push_empty(car_list, sizeof(car_t));

        /* Generate car: */
        car_t *new_car = (car_t *)car_node->data;
        generate_car(new_car);

        /* Generate thread to simulate car: */
        pthread_create(&new_car->sim_thread, NULL, car_sim_loop, new_car);

        /* Sleep for random time: */
        delay_random_ms(1, 100);
    }
}

int car_compare_lplate(char *lplate1[license_plate_lenth], char *lplate2[license_plate_lenth])
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

void car_data_destroy(void *car_data)
{
    car_t *car = (car_t *)car_data;

    /* Join car thread: */
    pthread_join(&car->sim_thread, NULL);
}

//////////////////// End car functionality and model.

int main(void)
{
    _quit = false;

    random_init(time(0));

    /* Setup shared memory and attach: */
    shared_mem_t *shared_mem;
    create_shared_object(shared_mem);
    shared_mem_attach(shared_mem);

    /* Initialise threading: */
        /* Entrances: */
    for(unsigned int i = 0; i < num_entrances; ++i)
    {
            /* Boom gate: */
        pthread_mutex_init(&shared_mem->data->entrances[i].bgate.bgate_mutex, NULL);
        pthread_cond_init(&shared_mem->data->entrances[i].bgate.bgate_update_flag, NULL);

            /* Information sign: */
        pthread_mutex_init(&shared_mem->data->entrances[i].info_sign.info_sign_mutex, NULL);
        pthread_cond_init(&shared_mem->data->entrances[i].info_sign.info_sign_update_flag, NULL);
        
            /* License plate sensor: */
        pthread_mutex_init(&shared_mem->data->entrances[i].lplate_sensor.lplate_sensor_mutex, NULL);
        pthread_cond_init(&shared_mem->data->entrances[i].lplate_sensor.lplate_sensor_update_flag, NULL);
    }
        /* Exits: */
    for(unsigned int i = 0; i < num_entrances; ++i)
    {
            /* Boom gate: */
        pthread_mutex_init(&shared_mem->data->exits[i].bgate.bgate_mutex, NULL);
        pthread_cond_init(&shared_mem->data->exits[i].bgate.bgate_update_flag, NULL);
        
            /* License plate sensor: */
        pthread_mutex_init(&shared_mem->data->exits[i].lplate_sensor.lplate_sensor_mutex, NULL);
        pthread_cond_init(&shared_mem->data->exits[i].lplate_sensor.lplate_sensor_update_flag, NULL);
    }
        /* Levels: */
    for(unsigned int i = 0; i < num_entrances; ++i)
    {   
            /* License plate sensor: */
        pthread_mutex_init(&shared_mem->data->levels[i].lplate_sensor.lplate_sensor_mutex, NULL);
        pthread_cond_init(&shared_mem->data->levels[i].lplate_sensor.lplate_sensor_update_flag, NULL);
    }

    /* Setup car generator thread: */
    pthread_t car_gen_thread;
    pthread_create(&car_gen_thread, NULL, generate_cars_loop, NULL);

    /* End of simulation: */
        /* Close all threads: */
    pthread_join(car_gen_thread, NULL);
    llist_close(car_list);

        /* Destroy shared memory: */
    destroy_shared_object(shared_mem);


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