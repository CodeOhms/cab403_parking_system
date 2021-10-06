#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct license_plate_sensor_t
{
    pthread_mutex_t lplate_sensor_mutex;
    pthread_cond_t lplate_sensor_update_flag;
    char license_plate[6];
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
    boom_gate_t boom_gate;
} exit_t;

typedef struct level_t
{
    license_plate_sensor_t lplate_sensor;
    uint16_t temp_sensor;
    bool alarm;
} level_t;

typedef struct shared_mem_t
{
    entrance_t entrances[5];
    exit_t exits[5];
    level_t levels[5];
} shared_mem_t;