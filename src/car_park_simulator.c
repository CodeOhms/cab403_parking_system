#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include "utils.h"
#include "shared_memory.h"
#include "linked_list.h"
#include "thread_pool.h"

#define NUM_BUCKETS 100

bool quit;
sem_t quit_sem;
shared_mem_t shared_mem;
shared_mem_t handshake_mem;
thread_pool_t car_thread_pool;
htab_t auth_vehicle_plates_htab;
char auth_lplates[TOTAL_CAPACITY][LICENSE_PLATE_LENGTH + 1];
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
    shm_unlink(shm->name);

    // Create the shared memory object, allowing read-write access, and saving the
    // resulting file descriptor in shm->fd. If creation failed, ensure 
    // that shm->data is NULL and return false.
    if((shm->fd = shm_open(shm->name, O_CREAT | O_RDWR, 0666)) < 0)
    {
        shm->data = NULL;
        return false;
    }

    // Set the capacity of the shared memory object via ftruncate. If the 
    // operation fails, ensure that shm->data is NULL and return false. 
    if(ftruncate(shm->fd, shm->size) == -1)
    {
        shm->data = NULL;
        return false;
    }

    // Otherwise, attempt to map the shared memory via mmap, and save the address
    // in shm->data. If mapping fails, return false.
    shm->data = mmap(0, shm->size, PROT_READ | PROT_WRITE, MAP_SHARED, shm->fd, 0);
    if(shm->data == MAP_FAILED)
    {
        return false;
    }

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
    free(shm->name);
    munmap(shm->data, shm->size);
    shm_unlink(shm->name);
    shm->fd = -1;
    shm->data = NULL;
}

int shm_data_init(pthread_mutexattr_t *mutex_attr, pthread_condattr_t *cond_attr)
{
    /* Create shared memory objects and attach: */
    shared_mem_data_init(&shared_mem, SHM_SIZE, SHM_NAME, SHM_NAME_LENGTH);
    if(!create_shared_object(&shared_mem))
    {
        return -1;
    }
    if(!shared_mem_attach(&shared_mem))
    {
        return -1;
    }
    shared_mem_data_init(&handshake_mem, SHM_LINK_MANAGER_SIZE, SHM_HANDSHAKE_NAME, SHM_HANDSHAKE_NAME_LENGTH);
    if(!create_shared_object(&handshake_mem))
    {
        return -1;
    }
    if(!shared_mem_attach(&handshake_mem))
    {
        return -1;
    }
    shared_data_t *shm_data = (shared_data_t *)shared_mem.data;
    shared_handshake_t *handshake_data = (shared_handshake_t *)handshake_mem.data;

    /* Initialise shared memory variables: */
    pthread_mutexattr_setpshared(mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(cond_attr, PTHREAD_PROCESS_SHARED);
        /* Handshake: */
    sem_init(&handshake_data->shm_mem_ready, 1, 0);
    sem_init(&handshake_data->manager_linked, 1, 0);
    sem_init(&handshake_data->simulator_finished, 1, 0);
    
        /* Entrances: */
    for(uint8_t i = 0; i < NUM_ENTRANCES; ++i)
    {
            /* Boom gate: */
        pthread_mutex_init(&shm_data->entrances[i].bgate.bgate_mutex, mutex_attr);
        pthread_cond_init(&shm_data->entrances[i].bgate.bgate_update_flag, cond_attr);

            /* Information sign: */
        pthread_mutex_init(&shm_data->entrances[i].info_sign.info_sign_mutex, mutex_attr);
        pthread_cond_init(&shm_data->entrances[i].info_sign.info_sign_update_flag, cond_attr);
        
            /* License plate sensor: */
        pthread_mutex_init(&shm_data->entrances[i].lplate_sensor.lplate_sensor_mutex, mutex_attr);
        pthread_cond_init(&shm_data->entrances[i].lplate_sensor.lplate_sensor_update_flag, cond_attr);
    }
        /* Exits: */
    for(uint8_t i = 0; i < NUM_EXITS; ++i)
    {
            /* Boom gate: */
        pthread_mutex_init(&shm_data->exits[i].bgate.bgate_mutex, mutex_attr);
        pthread_cond_init(&shm_data->exits[i].bgate.bgate_update_flag, cond_attr);
        
            /* License plate sensor: */
        pthread_mutex_init(&shm_data->exits[i].lplate_sensor.lplate_sensor_mutex, mutex_attr);
        pthread_cond_init(&shm_data->exits[i].lplate_sensor.lplate_sensor_update_flag, cond_attr);
    }
        /* Levels: */
    for(uint8_t i = 0; i < NUM_LEVELS; ++i)
    {   
            /* License plate sensor: */
        pthread_mutex_init(&shm_data->levels[i].lplate_sensor.lplate_sensor_mutex, mutex_attr);
        pthread_cond_init(&shm_data->levels[i].lplate_sensor.lplate_sensor_update_flag, cond_attr);
    }

    /* Signal to the manager that the shared memory is ready: */
    sem_post(&handshake_data->shm_mem_ready);

    return 0;
}

void shm_data_close(pthread_mutexattr_t *mutex_attr, pthread_condattr_t *cond_attr)
{
    shared_data_t *shm_data = (shared_data_t *)shared_mem.data;
    shared_handshake_t *handshake_data = (shared_handshake_t *)handshake_mem.data;
    
    /* Destroy mutex and condition attribute variables: */
    pthread_mutexattr_destroy(mutex_attr);
    pthread_condattr_destroy(cond_attr);

        /* Destroy mutexes, condition variables and semaphores: */
            /* Handshake: */
    sem_destroy(&handshake_data->shm_mem_ready);
    sem_destroy(&handshake_data->manager_linked);
    sem_destroy(&handshake_data->simulator_finished);

                /* Entrances: */
    for(uint8_t i = 0; i < NUM_ENTRANCES; ++i)
    {
            /* Boom gate: */
        pthread_mutex_destroy(&shm_data->entrances[i].bgate.bgate_mutex);
        pthread_cond_destroy(&shm_data->entrances[i].bgate.bgate_update_flag);

            /* Information sign: */
        pthread_mutex_destroy(&shm_data->entrances[i].info_sign.info_sign_mutex);
        pthread_cond_destroy(&shm_data->entrances[i].info_sign.info_sign_update_flag);
        
            /* License plate sensor: */
        pthread_mutex_destroy(&shm_data->entrances[i].lplate_sensor.lplate_sensor_mutex);
        pthread_cond_destroy(&shm_data->entrances[i].lplate_sensor.lplate_sensor_update_flag);
    }
            /* Exits: */
    for(uint8_t i = 0; i < NUM_EXITS; ++i)
    {
            /* Boom gate: */
        pthread_mutex_destroy(&shm_data->exits[i].bgate.bgate_mutex);
        pthread_cond_destroy(&shm_data->exits[i].bgate.bgate_update_flag);
        
            /* License plate sensor: */
        pthread_mutex_destroy(&shm_data->exits[i].lplate_sensor.lplate_sensor_mutex);
        pthread_cond_destroy(&shm_data->exits[i].lplate_sensor.lplate_sensor_update_flag);
    }
            /* Levels: */
    for(uint8_t i = 0; i < NUM_LEVELS; ++i)
    {   
            /* License plate sensor: */
        pthread_mutex_destroy(&shm_data->levels[i].lplate_sensor.lplate_sensor_mutex);
        pthread_cond_destroy(&shm_data->levels[i].lplate_sensor.lplate_sensor_update_flag);
    }
}

//////////////////// End shared memory functionality.

//////////////////// Handshake functionality:

void wait_for_manager(shared_handshake_t *handshake)
{
    sem_wait(&handshake->manager_linked);
}

//////////////////// End handshake functionality.

//////////////////// Information sign functionality:

void info_sign_read(information_sign_t *info_sign, char *display)
{
    pthread_mutex_lock(&info_sign->info_sign_mutex);
    pthread_cond_wait(&info_sign->info_sign_update_flag, &info_sign->info_sign_mutex);
    *display = info_sign->display;
    pthread_mutex_unlock(&info_sign->info_sign_mutex);
}

//////////////////// End information sign functionality.

//////////////////// Boom gate functionality:

/**
 * @brief Handles simulating a boom gate.
 */
void boom_gate_loop(void *args)
{
    boom_gate_t *bgate = (boom_gate_t *)args;

    bgate->bgate_state = C;

    pthread_mutex_lock(&bgate->bgate_mutex);

    while(!quit)
    {
        pthread_cond_wait(&bgate->bgate_update_flag, &bgate->bgate_mutex);

        /* State machine: */
        switch (bgate->bgate_state)
        {
            case R:
                /* Raising: */
                delay_ms(10, time_scale);
                bgate->bgate_state = O;
                break;

            case L:
                /* Lowering: */
                delay_ms(10, time_scale);
                bgate->bgate_state = C;
                break;
            
            default:
                break;
        }
    }

     pthread_mutex_unlock(&bgate->bgate_mutex);
}

void boom_gate_wait_open(boom_gate_t *bgate)
{
    pthread_mutex_lock(&bgate->bgate_mutex);

    /* Wait for boom gate to open: */
    pthread_cond_wait(&bgate->bgate_update_flag, &bgate->bgate_mutex);

    pthread_mutex_unlock(&bgate->bgate_mutex);
}

//////////////////// End boom gate functionality.

//////////////////// License plate sensor functionality:

void lplate_sensor_trigger(license_plate_sensor_t *lps, char* lplate)
{
    pthread_mutex_lock(&lps->lplate_sensor_mutex);
    strcpy(lps->license_plate, lplate);
    pthread_mutex_unlock(&lps->lplate_sensor_mutex);
    pthread_cond_signal(&lps->lplate_sensor_update_flag);
}

//////////////////// End license plate sensor functionality.

//////////////////// Car functionality and model:

typedef struct car_t
{
    char license_plate[LICENSE_PLATE_LENGTH];
    uint8_t level_assigned;
    // pthread_t sim_thread;
} car_t;

list_t *car_list;
pthread_mutex_t car_list_mutex;

typedef struct entrance_queues_sh_data_t
{
    list_t *queue[NUM_ENTRANCES];
    sem_t full[NUM_ENTRANCES];
    sem_t cars_simulating;
    pthread_mutex_t mutex[NUM_ENTRANCES];
} entrance_queues_sh_data_t;

typedef struct entrance_queue_t
{
    entrance_queues_sh_data_t *sh_data;
    uint8_t entrance_num;
} entrance_queue_t;

typedef struct car_info_t
{
    node_t *car_node;
    entrance_queue_t *e_queue;
    pthread_cond_t *entrance_leave;
    pthread_mutex_t *occupy_mutex;
} car_management_info_t;

void *car_simulation_loop(void *args);

void entrance_queue_init(entrance_queues_sh_data_t *e_q_sh_data)
{
    /* Initialise the linked lists: */
    for(uint8_t e = 0; e < NUM_ENTRANCES; ++e)
    {
        llist_init(&e_q_sh_data->queue[e], NULL, NULL);
    }

    /* Initialise the semaphores: */
    for(uint8_t e = 0; e < NUM_ENTRANCES; ++e)
    {
        sem_init(&e_q_sh_data->full[e], 0, 0);
    }
    sem_init(&e_q_sh_data->cars_simulating, 0, 0);

    /* Initialise the mutexes: */
    for(uint8_t e = 0; e < NUM_ENTRANCES; ++e)
    {
        pthread_mutex_init(&e_q_sh_data->mutex[e], NULL);
    }
}

void entrance_queue_close(entrance_queues_sh_data_t *e_q_sh_data)
{
    for(uint8_t e = 0; e < NUM_ENTRANCES; ++e)
    {
        llist_close(e_q_sh_data->queue[e]);
    }

    for(uint8_t e = 0; e < NUM_ENTRANCES; ++e)
    {
        sem_destroy(&e_q_sh_data->full[e]);
    }
    sem_destroy(&e_q_sh_data->cars_simulating);

    for(uint8_t e = 0; e < NUM_ENTRANCES; ++e)
    {
        pthread_mutex_destroy(&e_q_sh_data->mutex[e]);
    }
}

void car_leave_entrance(car_management_info_t *car_man_info, uint8_t entrance_num)
{
    pthread_mutex_unlock(car_man_info->occupy_mutex);
    pthread_cond_signal(car_man_info->entrance_leave);
}

/**
 * @brief A function intended to monitor a single entrance to ensure one car is processed at a time.
 * Must be run in its own thread.
 * 
 * Note that is is implemented using the Bounded Buffer design pattern where each entrance acts
 * as a single slot buffer. As it is a single slot buffer the 'empty' semaphore is binary, which
 * is the same as a mutex.
 */
void *manage_entrances_loop(void *args)
{
    entrance_queue_t *e_queue = (entrance_queue_t *)args;
    entrance_queues_sh_data_t *e_queues_data = e_queue->sh_data;
    uint8_t e_id = e_queue->entrance_num;

    /* Variables needed to ensure one car processed at a time: */
    pthread_mutex_t occupy_mutex;
    pthread_cond_t finished;
    pthread_mutex_init(&occupy_mutex, NULL);
    pthread_cond_init(&finished, NULL);

    do
    {
        /* Wait until at least one car is at the entrance: */
        sem_wait(&e_queues_data->full[e_id]);
        if(quit)
        { /* Main thread pretending a car has arrived to wake us up: */
            break;
        }
        pthread_mutex_lock(&occupy_mutex);

        pthread_mutex_lock(&e_queues_data->mutex[e_id]);

        /* Allow next car in: */
            /* Remove car from queue: */
        node_t *old_next_car_node = llist_pop(e_queues_data->queue[e_id]);
        pthread_mutex_unlock(&e_queues_data->mutex[e_id]);

            /* Put into linked list of existing cars: */
        pthread_mutex_lock(&car_list_mutex);
        node_t *next_car_node = llist_push(car_list, old_next_car_node->data, sizeof(car_t));
        pthread_mutex_unlock(&car_list_mutex);

        /* Note `llist_push()` will shallow copy the node data, so free the old copy: */
        llist_delete_dangling_node(old_next_car_node, NULL);
        
        
        /* Spin off a thread for it: */
        car_management_info_t *car_man_info = (car_management_info_t *)malloc(sizeof(car_management_info_t));
        car_man_info->car_node = next_car_node;
        car_man_info->e_queue = e_queue;
        car_man_info->entrance_leave = &finished;
        car_man_info->occupy_mutex = &occupy_mutex;
        thread_pool_add_request(&car_thread_pool, car_simulation_loop, car_man_info);

        /* Wait for the car thread that was just started to leave the entrance.
           Car thread will hold the mutex until it has left the entrance. */
        pthread_cond_wait(&finished, &occupy_mutex);
        pthread_mutex_unlock(&occupy_mutex);
    } while(!quit);

    pthread_mutex_destroy(&occupy_mutex);
    pthread_cond_destroy(&finished);

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
    if(random_int(&random_gen_mutex, 0, 1) == 0)
    {
        do
        {
            generate_license_plate(lplate);
        } while (llist_find(car_list, lplate) != NULL);
    }
    else
    {
        item_t *auth_car;
        do
        {
            generate_license_plate(lplate);
            auth_car = htab_bucket(&auth_vehicle_plates_htab, lplate);
        } while (auth_car == NULL);
        strcpy(lplate, auth_car->key);
    }
}

/**
 * @brief Generate a car and place it into the queue of a random entrance.
 */
void generate_and_queue_car(entrance_queues_sh_data_t *e_queue_sh_data, uint8_t entrance_num)
{
    /* Create node in linked list for a new car: (This will allocate memory for new car) */
    node_t *car_node = llist_append_empty(e_queue_sh_data->queue[entrance_num], sizeof(car_t));
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

size_t cars_sim_ended;
pthread_mutex_t cars_sim_ended_mutex;
pthread_cond_t cars_sim_ended_cond;
void *car_simulation_loop(void *args)
{
    shared_data_t *shm_data = (shared_data_t *)shared_mem.data;

    car_management_info_t *car_man_info = (car_management_info_t *)args;
    car_t *car_data = (car_t *)car_man_info->car_node->data;
    entrance_queues_sh_data_t *e_queues_data = car_man_info->e_queue->sh_data;
    uint8_t en_id = car_man_info->e_queue->entrance_num;

    /* Lock the entrance occupied mutex: */
    pthread_mutex_lock(car_man_info->occupy_mutex);

    /* Wait a bit before triggering the LPS: */
    delay_ms(2, time_scale);
    lplate_sensor_trigger(&shm_data->entrances[en_id].lplate_sensor, car_data->license_plate);

    /* Get information from digital sign: */
    char display = 'X';
    info_sign_read(&shm_data->entrances[en_id].info_sign, &display);

    /* Respond to information received from sign (if digit given continue, else rejected): */
    if('0' <= display || display <= '9')
    { /* Car allowed. */
    /* Deviant behaviour (30% chance): */
        uint8_t level = atoi(&display);

    /* Wait for boom gate to open: */
        boom_gate_wait_open(&shm_data->entrances[en_id].bgate);

    /* Car finished with the LPS and ready to enter, unlock occupy mutex.
       Also, signal that entrance is available to the next car: */
        car_leave_entrance(car_man_info, en_id);

    /* Continue into the car park: */
        /* Go to assigned level and trigger level LPS: */
        lplate_sensor_trigger(&shm_data->levels[level].lplate_sensor, car_data->license_plate);

    /* Stay in car park for a random period of time (between 100-10,000 ms): */
        delay_random_ms(&random_gen_mutex, 100, 10000, time_scale);

    /* Leave after finish parking, triggering level LPS and exit LPS: */
        uint8_t ex_id = random_int(&random_gen_mutex, 0, NUM_EXITS - 1);
        lplate_sensor_trigger(&shm_data->exits[ex_id].lplate_sensor, car_data->license_plate);
    }
    else
    { /* Car rejected. */
        car_leave_entrance(car_man_info, en_id);
    }

    /* Remove car from simulation: */
    pthread_mutex_lock(&car_list_mutex);
    llist_delete_node(car_list, car_man_info->car_node);
    pthread_mutex_unlock(&car_list_mutex);

    /* Signal to car generator that car has finished simulating: */
    pthread_mutex_lock(&cars_sim_ended_mutex);
    ++cars_sim_ended;
    sem_post(&e_queues_data->cars_simulating);
    pthread_mutex_unlock(&cars_sim_ended_mutex);

    /* Free memory for car info, but not its contents: */
    free(car_man_info);

    return NULL;
}

void *generate_cars_loop(void *args)
{
    entrance_queues_sh_data_t *e_q_sh_data = (entrance_queues_sh_data_t *)args;

    // TODO: have ability to limit number of cars in the simulation from cmd line:
    size_t cars_to_sim = 20;
    size_t cars_sim_started = 0;
    cars_sim_ended = 0;
    
    do
    {
        /* Chose a random entrance to queue at: */
        uint8_t entrance_num = random_int(&random_gen_mutex, 0, NUM_ENTRANCES - 1);
        pthread_mutex_lock(&e_q_sh_data->mutex[entrance_num]);
        generate_and_queue_car(e_q_sh_data, entrance_num);
        ++cars_sim_started;
        pthread_mutex_unlock(&e_q_sh_data->mutex[entrance_num]);
        sem_post(&e_q_sh_data->full[entrance_num]);

        /* Sleep for random time: */
        delay_random_ms(&random_gen_mutex, 1, 100, time_scale);

        if(cars_sim_started >= cars_to_sim)
        { /* Stop generating new cars */
            /* Wait for all cars to finish simulating: */
            while(true)
            {
                pthread_mutex_lock(&cars_sim_ended_mutex);
                if(cars_sim_ended >= cars_to_sim - 1)
                {
                    quit = true;
                    sem_post(&quit_sem);
                    pthread_mutex_unlock(&cars_sim_ended_mutex);
                    break;
                }
                pthread_mutex_unlock(&cars_sim_ended_mutex);
                sem_wait(&e_q_sh_data->cars_simulating);
            }
        }
    } while(!quit);

    return NULL;
}

//////////////////// End car functionality and model.

int main(void)
{
    quit = false;
    sem_init(&quit_sem, 0, 0);

    random_init(&random_gen_mutex, time(0));

    htab_init(&auth_vehicle_plates_htab, NUM_BUCKETS);
    lp_list(&auth_vehicle_plates_htab, auth_lplates);

    /* Initialise shared memory: */
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;
    {
        int ret = shm_data_init(&mutex_attr, &cond_attr);
        if(ret != 0)
        {
            return ret;
        }
    }

    /* Wait for the manager to open: */
    shared_handshake_t *handshake_data = (shared_handshake_t *)handshake_mem.data;
    wait_for_manager(handshake_data);

    /* Initialise threading: */
        /* Initialise variables needed for threads: */
    llist_init(&car_list, car_compare_lplate, NULL);
    entrance_queues_sh_data_t entrance_queues_sh_data;
    entrance_queue_init(&entrance_queues_sh_data);
    entrance_queue_t entrance_queues[NUM_ENTRANCES];
    for(uint8_t e = 0; e < NUM_ENTRANCES; ++e)
    {
        entrance_queues[e].sh_data = &entrance_queues_sh_data;
        entrance_queues[e].entrance_num = e;
    }

        /* Setup thread pool for cars: */
    thread_pool_init(&car_thread_pool);

        /* Setup car generator thread: */
    pthread_t car_gen_thread;
    pthread_create(&car_gen_thread, NULL, generate_cars_loop, (void *)&entrance_queues_sh_data);

        /* Setup car entrance queue manager thread: */
    pthread_t manage_entrances_threads[NUM_ENTRANCES];
    pthread_mutex_init(&car_list_mutex, NULL);
    for(uint8_t e = 0; e < NUM_ENTRANCES; ++e)
    {
        pthread_create(&manage_entrances_threads[e], NULL, manage_entrances_loop, (void *)&entrance_queues[e]);
    }

    /* End of simulation: */
        /* Signal to manager it's closing time: */
    sem_wait(&quit_sem);
    sem_post(&handshake_data->simulator_finished);

    /* Close all threads: */
    thread_pool_close(&car_thread_pool);
    for(uint8_t e = 0; e < NUM_ENTRANCES; ++e)
    {
        /* Ensure no thread is stuck waiting for a car via the `full` semaphore: */
        sem_post(&entrance_queues_sh_data.full[e]);

        pthread_join(manage_entrances_threads[e], NULL);
    }
    pthread_join(car_gen_thread, NULL);
    entrance_queue_close(&entrance_queues_sh_data);

        /* Destroy shared memory: */
    shm_data_close(&mutex_attr, &cond_attr);
    
    destroy_shared_object(&shared_mem);
    destroy_shared_object(&handshake_mem);

    htab_destroy(&auth_vehicle_plates_htab);

    random_close(&random_gen_mutex);
    sem_destroy(&quit_sem);
}