#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include "utils.h"
#include "shared_memory.h"
#include "linked_list.h"
#include "thread_pool.h"

bool _quit;
shared_mem_t shared_mem;
thread_pool_t car_thread_pool;
pthread_mutex_t random_gen_mutex;
unsigned int time_scale = 1;

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

//////////////////// Car functionality and model:

typedef struct car_t
{
    char license_plate[LICENSE_PLATE_LENGTH];
    uint8_t level_assigned;
    // pthread_t sim_thread;
} car_t;

list_t *car_list;
pthread_mutex_t car_list_mutex;

typedef struct entrance_queue_t
{
    list_t *queue[NUM_ENTRANCES];
    bool is_busy[NUM_ENTRANCES];
    pthread_mutex_t mutex;
    pthread_cond_t busy;
} entrance_queue_t;

typedef struct car_info_t
{
    node_t *car_node;
    entrance_queue_t *entrance_queue;
    uint8_t entrance_num;
} car_info_t;

void *car_simulation_loop(void *args);

void entrance_queue_init(entrance_queue_t *entrance_queues)
{
    /* Initialise the linked lists: */
    for(uint8_t e = 0; e < NUM_ENTRANCES; ++e)
    {
        llist_init(&entrance_queues->queue[e], NULL, NULL);
    }

    /* Initialise the mutex and condition variables: */
    pthread_mutex_init(&entrance_queues->mutex, NULL);
    pthread_cond_init(&entrance_queues->busy, NULL);
}

void entrance_queue_close(entrance_queue_t *entrance_queues)
{
    for(uint8_t e = 0; e < NUM_ENTRANCES; ++e)
    {
        llist_close(entrance_queues->queue[e]);
    }

    pthread_mutex_destroy(&entrance_queues->mutex);
    pthread_cond_destroy(&entrance_queues->busy);
}

void car_leave_entrance(entrance_queue_t *entrance_queues, uint8_t entrance_num)
{
    pthread_mutex_lock(&entrance_queues->mutex);
    entrance_queues->is_busy[entrance_num] = false;
    pthread_mutex_unlock(&entrance_queues->mutex);
    pthread_cond_signal(&entrance_queues->busy);
}

void *manage_entrances_loop(void *args)
{
    entrance_queue_t *entrance_queues = (entrance_queue_t *)args;

    while(!_quit)
    {
        pthread_mutex_lock(&entrance_queues->mutex);
        
        pthread_cond_wait(&entrance_queues->busy, &entrance_queues->mutex);
        /* Check each entrance and allow the next car in if available: */
        for(uint8_t e = 0; e < NUM_ENTRANCES; ++e)
        {
            if(!entrance_queues->is_busy[e])
            {
                /* Allow next car in: */
                entrance_queues->is_busy[e] = true;
                    /* Put into linked list of existing cars: */
                node_t *old_next_car_node = llist_pop(entrance_queues->queue[e]);
                node_t *next_car_node = llist_push(car_list, next_car_node->data, sizeof(car_t));

                    /* Note `llist_push()` will shallow copy the node data, so free the old copy: */
                llist_delete_dangling_node(old_next_car_node, NULL);
                
                    /* Spin off a thread for it: */
                car_info_t *car_info = (car_info_t *)malloc(sizeof(car_info_t));
                car_info->car_node = next_car_node;
                car_info->entrance_queue = entrance_queues;
                car_info->entrance_num = e;
                thread_pool_add_request(&car_thread_pool, car_simulation_loop, car_info);
            }
        }
    }

    pthread_mutex_unlock(&entrance_queues->mutex);

    return NULL;
}

void generate_license_plate(char *lplate)
{
    /* Generate numbers: */
    for(uint8_t i = 0; i < LICENSE_PLATE_LENGTH/2; ++i)
    {
        lplate[i] = random_digit(&random_gen_mutex);
    }

    /* Generate letters (Capitalised ASCII): */
    for(uint8_t i = LICENSE_PLATE_LENGTH/2; i < LICENSE_PLATE_LENGTH; ++i)
    {
        lplate[i] = random_letter(&random_gen_mutex);
    }
}

void generate_unique_license_plate(char *lplate)
{
    /* Ensure car doesn't currently exist (no license plate duplicates): */
    generate_license_plate(lplate);
    while(llist_find(car_list, lplate) != NULL)
    {
        generate_license_plate(lplate);
    }
}

/**
 * @brief Generate a car and place it into the queue of a random entrance.
 */
void generate_and_queue_car(entrance_queue_t *entrance_queues)
{   
    /* Chose a random entrance to queue at: */
    uint8_t entrance_num = random_int(&random_gen_mutex, 0, NUM_ENTRANCES - 1);
    
    /* Create node in linked list for a new car: (This will allocate memory for new car) */
    node_t *car_node = llist_append_empty(entrance_queues->queue[entrance_num], sizeof(car_t));
    car_t *new_car = (car_t *)car_node->data;
    
    /* Generate license plate: */
    generate_unique_license_plate(new_car->license_plate);
}

int car_compare_lplate(const void *lplate1, const void *car)
{
    car_t *_car = (car_t *)car;
    char *lplate2 = (char *)_car->license_plate;
    /* When a car is generated, its node will have a license plate of null term chars so ignore it: */
    if(lplate2[0] != 0)
    {
        // if(strcmp(_lplate1, _lplate2) == 0)
        if(memcmp(lplate1, lplate2, LICENSE_PLATE_LENGTH) == 0)
        { /* Exact match. */
            return 0;
        }
    // else if()
    // { /* Before alphabetically and numerically: */
    //     /* TODO: Implement functionality to compare license plates in ascending order! */
    // }
    }

    /* Once compare function complete, delete below return statement! */
    return -1;
}

// void car_data_destroy(void *car_data)
// {
//     car_t *car = (car_t *)car_data;

//     /* Join car thread: */
//     pthread_join(car->sim_thread, NULL);
// }

void *car_simulation_loop(void *args)
{
    car_info_t *car_info = (car_info_t *)args;
    node_t *car_node = car_info->car_node;

    /* Aquire mutex for the entrance LPS: */

    /* Wait a bit before triggering the LPS: */

    /* Update the LPS license plate field, then trigger LPS cond. var. for the entrance: */
    pthread_cond_signal(&shared_mem.data->entrances[0].lplate_sensor.lplate_sensor_update_flag);

    /* Car finsihed with the LPS, unlock mutex: */
    

    /* Get information from digital sign: */
    char display = ' ';
        /* Aquire mutex for the sign: */

        /* Wait for sign to update: */

        /* Unlock mutex for the sign: */

    /* Respond to information received from sign (if digit given continue, else rejected): */
    if('0' <= display || display <= '9')
    { /* Car allowed. */
    /* Wait for boom gate to open: */

    /* Continue into the car park: */
        /* Car finished with the LPS and ready to enter, unlock mutex.
           Also, signal that entrance is available to the next car: */
        car_leave_entrance(car_info->entrance_queue, car_info->entrance_num);

    /* Go to assigned level and trigger level LPS: */

    /* Stay in car park for a random period of time (between 100-10,000 ms): */
    delay_random_ms(&random_gen_mutex, 100, 10000, time_scale);

    /* Leave after finish parking, triggering level LPS and exit LPS: */ 

    }
    else
    { /* Car rejected. */
        car_leave_entrance(car_info->entrance_queue, car_info->entrance_num);
    }

    /* Remove car from simulation: */
    pthread_mutex_lock(&car_list_mutex);
    llist_delete_node(car_list, car_node);
    pthread_mutex_unlock(&car_list_mutex);

    /* Free memory for car info, but not its contents (DO IT LAST): */
    free(car_info);

    return NULL;
}

void *generate_cars_loop(void *args)
{
    entrance_queue_t *entrance_queues = (entrance_queue_t *)args;

    // TODO: have ability to limit number of cars in the simulation from cmd line:
    size_t cars_to_sim = 10;
    size_t cars_simulated = 0;

    while(!_quit)
    {
        pthread_mutex_lock(&entrance_queues->mutex);
        generate_and_queue_car(entrance_queues);
        pthread_mutex_unlock(&entrance_queues->mutex);

        /* Sleep for random time: */
        delay_random_ms(&random_gen_mutex, 1, 100, time_scale);

        if(cars_simulated >= cars_to_sim)
        {
            _quit = true;
        }
        ++cars_simulated;
    }

    /* Ensure all car threads have been closed: */
    llist_close(car_list);

    return NULL;
}

//////////////////// End car functionality and model.

int main(void)
{
    _quit = false;

    random_init(&random_gen_mutex, time(0));

    /* Initialise shared memory: */
        /* Create shared memory object and attach: */
    create_shared_object(&shared_mem);
    shared_mem_attach(&shared_mem);

    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
        /* Entrances: */
    for(uint8_t i = 0; i < NUM_ENTRANCES; ++i)
    {
            /* Boom gate: */
        pthread_mutex_init(&shared_mem.data->entrances[i].bgate.bgate_mutex, &mutex_attr);
        pthread_cond_init(&shared_mem.data->entrances[i].bgate.bgate_update_flag, &cond_attr);

            /* Information sign: */
        pthread_mutex_init(&shared_mem.data->entrances[i].info_sign.info_sign_mutex, &mutex_attr);
        pthread_cond_init(&shared_mem.data->entrances[i].info_sign.info_sign_update_flag, &cond_attr);
        
            /* License plate sensor: */
        pthread_mutex_init(&shared_mem.data->entrances[i].lplate_sensor.lplate_sensor_mutex, &mutex_attr);
        pthread_cond_init(&shared_mem.data->entrances[i].lplate_sensor.lplate_sensor_update_flag, &cond_attr);
    }
        /* Exits: */
    for(uint8_t i = 0; i < NUM_EXITS; ++i)
    {
            /* Boom gate: */
        pthread_mutex_init(&shared_mem.data->exits[i].bgate.bgate_mutex, &mutex_attr);
        pthread_cond_init(&shared_mem.data->exits[i].bgate.bgate_update_flag, &cond_attr);
        
            /* License plate sensor: */
        pthread_mutex_init(&shared_mem.data->exits[i].lplate_sensor.lplate_sensor_mutex, &mutex_attr);
        pthread_cond_init(&shared_mem.data->exits[i].lplate_sensor.lplate_sensor_update_flag, &cond_attr);
    }
        /* Levels: */
    for(uint8_t i = 0; i < NUM_LEVELS; ++i)
    {   
            /* License plate sensor: */
        pthread_mutex_init(&shared_mem.data->levels[i].lplate_sensor.lplate_sensor_mutex, &mutex_attr);
        pthread_cond_init(&shared_mem.data->levels[i].lplate_sensor.lplate_sensor_update_flag, &cond_attr);
    }

    /* Initialise threading: */
        /* Setup thread pool for cars: */
    thread_pool_init(&car_thread_pool);

        /* Setup car generator thread: */
    entrance_queue_t entrance_queues;
    entrance_queue_init(&entrance_queues);
    pthread_t car_gen_thread;
    pthread_create(&car_gen_thread, NULL, generate_cars_loop, (void *)&entrance_queues);

        /* Setup car entrance queue manager thread: */
    pthread_mutex_init(&car_list_mutex, NULL);
    llist_init(&car_list, car_compare_lplate, NULL);
    pthread_t manage_entrances_thread;
    pthread_create(&manage_entrances_thread, NULL, manage_entrances_loop, (void *)&entrance_queues);

    /* End of simulation: */
        /* Close all threads: */
    pthread_join(car_gen_thread, NULL);
    thread_pool_close(&car_thread_pool);

        /* Destroy mutex and condition attribute variables: */
    pthread_mutexattr_destroy(&mutex_attr);
    pthread_condattr_destroy(&cond_attr);

        /* Destroy shared memory: */
    destroy_shared_object(&shared_mem);

    random_close(&random_gen_mutex);
}