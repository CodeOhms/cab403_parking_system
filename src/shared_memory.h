#ifndef  SHARED_MEMORY_H
#define  SHARED_MEMORY_H
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>

// #define LICENSE_PLATE_LENGTH 6
// const int license_plate_lenth = 6;
#define license_plate_lenth 6
// const int num_entrances = 5;
// const int num_exits = 5;
// const int num_levels = 5;
#define num_entrances 5
#define num_exits 5
#define num_levels 5

typedef struct license_plate_sensor_t
{
    pthread_mutex_t lplate_sensor_mutex;
    pthread_cond_t lplate_sensor_update_flag;
    char license_plate[license_plate_lenth];
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
    char display[1];
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
    entrance_t entrances[num_entrances];
    exit_t exits[num_exits];
    level_t levels[num_levels];
} shared_data_t;

/* Structure to manage the shared memory data */
typedef struct shared_mem_t
{
    /* File descriptor of shared memory object: */
    int fd;

    /* Pointer to the shared data structure: */
    shared_data_t *data;
} shared_mem_t;

// const char* shm_name = "PARKING";
// const size_t shm_size = sizeof(shared_data_t);
#define shm_name "PARKING"
#define shm_size sizeof(shared_data_t)

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