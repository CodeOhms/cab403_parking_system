#ifndef  SHARED_MEMORY_H
#define  SHARED_MEMORY_H
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <fcntl.h>

#define LICENSE_PLATE_LENGTH 6
#define NUM_ENTRANCES 5
#define NUM_EXITS 5
#define NUM_LEVELS 5
#define FLOOR_CAPACITY 20
#define TOTAL_CAPACITY FLOOR_CAPACITY*NUM_LEVELS
#define SEM_LOCAL 0
#define SEM_SHARED 1

typedef struct license_plate_sensor_t
{
    pthread_mutex_t lplate_sensor_mutex;
    pthread_cond_t lplate_sensor_update_flag;
    char license_plate[LICENSE_PLATE_LENGTH];
} license_plate_sensor_t;

typedef enum boom_gate_state_t
{
    C,
    O,
    R,
    L
} boom_gate_state_t;

typedef struct boom_gate_t
{
    pthread_mutex_t bgate_mutex;
    pthread_cond_t bgate_update_flag;
    boom_gate_state_t bgate_state;
} boom_gate_t;

typedef struct information_sign_t
{
    pthread_mutex_t info_sign_mutex;
    pthread_cond_t info_sign_update_flag;
    char display;
} information_sign_t;

typedef struct entrance_t
{
    license_plate_sensor_t lplate_sensor;
    boom_gate_t bgate;
    information_sign_t info_sign;
} entrance_t;

typedef struct exit_t
{
    license_plate_sensor_t lplate_sensor;
    boom_gate_t bgate;
} exit_t;

typedef struct level_t
{
    license_plate_sensor_t lplate_sensor;
    volatile uint16_t temp_sensor;
    volatile bool alarm;
} level_t;

typedef struct shared_data_t
{
    entrance_t entrances[NUM_ENTRANCES];
    exit_t exits[NUM_EXITS];
    level_t levels[NUM_LEVELS];
} shared_data_t;

typedef struct shared_handshake_t
{
    sem_t shm_mem_ready;
    sem_t manager_linked;
    sem_t simulator_closing;
    sem_t manager_finished;
    // sem_t simulator_finished;
    // sem_t manager_closed;
    // bool sim_started;
    // bool sim_closed;
} shared_handshake_t;

/* Structure to manage the shared memory data */
typedef struct shared_mem_t
{
    /* File descriptor of shared memory object: */
    int fd;

    /* Name of the shared memory object: */
    char *name;

    size_t size;

    /* Pointer to the shared data structure: */
    void *data;
} shared_mem_t;

#define SHM_NAME "PARKING"
#define SHM_NAME_LENGTH sizeof(SHM_NAME)/sizeof(SHM_NAME[0])
#define SHM_SIZE sizeof(shared_data_t)
#define SHM_HANDSHAKE_NAME "LINK"
#define SHM_HANDSHAKE_NAME_LENGTH sizeof(SHM_HANDSHAKE_NAME)/sizeof(SHM_HANDSHAKE_NAME[0])
#define SHM_LINK_MANAGER_SIZE sizeof(shared_handshake_t)

/**
 * @brief Will initialise varaibles in the given shared_mem_t object.
 * Call before using `shared_mem_attach()`.
 */
void shared_mem_data_init(shared_mem_t* shm, size_t size, char *name, size_t name_length);

/**
 * @brief Attach to shared memory. If it already exists from a previous
 * instance of the simulation then it will overwrite it.
 * 
 * @param shm A pointer to type of shared memory object. This will be allocated
 * memory and shared memory object instantiated.
 * 
 * @return Boolen indicating success or failure of attaching to shared memory.
 */
bool shared_mem_attach(shared_mem_t* shm);

#endif //SHARED_MEMORY_H