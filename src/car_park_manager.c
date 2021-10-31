#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include "utils.h"
#include "shared_memory.h"
#include "linked_list.h"
#include "htab.h"
#include "thread_pool.h"
#include "manage_hardware.h"

#define FLOOR_CAPACITY 20
#define NUM_LEVELS 5

// Create variable
char license_plate[100][7];
int vehicle_tracker[100];
int license_plate_length = 0;
double start_time[100];

// Display 
int revenue = 0;
int vehicle_counter_floor[NUM_LEVELS];
int vehicle_counter_total;

char entrance_lps_current[NUM_ENTRANCES][7];
char exit_lps_current[NUM_EXITS][7];
char level_lps_current[NUM_LEVELS][7];

shared_mem_t shared_mem;
shared_mem_t handshake_mem;

htab_t vehicle_table;

bool quit;
sem_t quit_sem;

//////////////////// Quit functionality:

/**
 * @brief Use this in a thread dedicated to waiting for the simulator to finish.
 * Once the simulator finishes the program should begin shutdown sequence.
 * 
 * @returns NULL.
 */
void *wait_sim_close(void *args)
{
    shared_handshake_t *handshake_data = (shared_handshake_t *)handshake_mem.data;
    sem_wait(&handshake_data->simulator_finished);

    quit = true;

    return NULL;
}

//////////////////// End quit functionality.

// Setup License plate for reading
void lp_list ( void ) {

    FILE *plate = fopen("plates.txt", "r");

    const unsigned MAX_LENGTH = 256;
    char buffer[7];

    // Assign Characters to Hash Table
    while (fgets(buffer, MAX_LENGTH, plate)) {

        for (int i = 0; i < 6; i++) {
        license_plate[license_plate_length][i] = buffer[i];
        }
        license_plate[license_plate_length][6] = '\0';

        htab_add(&vehicle_table, license_plate[license_plate_length], license_plate_length+1);

        license_plate_length++;


    }

    // close the file
    fclose(plate);

}

// Function for Scanning For license Plate
int lp_scan(char license[6]){

        // Check if characters match
        if(htab_find(&vehicle_table, license))
        {
            printf("Match Found \n");
            return 1;
        }


    printf("No Match \n");
    return 0;
}

// Function for calculating bill
double calculate_bill(double start_time) {

    // Get Current Time
    struct timeval time;
    gettimeofday(&time,NULL);

    // Calculate time spent
    double time_ms = time.tv_sec * 1000 + time.tv_usec / 10000;
    double time_spent = time_ms-start_time;

    // Calculate and return bill
    return time_spent * 0.05;
}

// Function for writing to txt file
void write_bill ( char license_plate[6], float bill){

    // File Pointer
    FILE *f = fopen("billing.txt", "a");
    if (f == NULL)
    {
        printf("unable to open billing.txt\n");
        exit(EXIT_FAILURE);
    }

    /* print some text */
    fprintf(f, "%s $%.2f\n", license_plate, bill);

    fclose(f);
}

// Function to Open Entrence Boom Gate
void *entrance_monitor(void *args) {

    uint8_t gate = *((uint8_t *)args);

    shared_data_t *shm_data = (shared_data_t *)shared_mem.data;
    
    char license[7];
    int floor_signal;

    struct timeval time;

    // Start For Loop
    do {
        
        // Wait for License Plate
        lplate_sensor_read(&shm_data->entrances[gate].lplate_sensor, license);
        // Check if there is space in car park
        if (vehicle_counter_total < FLOOR_CAPACITY*NUM_LEVELS){

        // Check if license plate is on list
            if (lp_scan(license) == 1) {

                // Get Value of License Plate
                int license_value = htab_find(&vehicle_table, license)->value;

        // Scan for Empty Floor
            for (int i = 0; i < NUM_LEVELS; i++){

                // If floor enough space, assign message,
                if (vehicle_counter_floor[i] < FLOOR_CAPACITY) {
                    floor_signal = i;
                    // Break Loop by changing i
                    i = NUM_LEVELS + 1;
                }
            }

        // Store time the Car in hash table
            // Calculate Time in MS
                gettimeofday(&time, NULL);
                double current_time_ms = time.tv_sec * 1000 + time.tv_usec / 10000;
                start_time[license_value] = current_time_ms;

        // Update Counter
                vehicle_counter_total++;

        // Signal Boom Gate to Open
                boom_gate_admit_one(&shm_data->entrances[gate].bgate);
            }
            else
            {
                info_sign_update(&shm_data->entrances->info_sign, 'X');
            }
        }
        else
        {
            info_sign_update(&shm_data->entrances->info_sign, 'F');
        }
    } while(!quit);

    free(args);

    return NULL;
}

// Function to open Exit Boom Gate
void *exit_monitor(void *args) {

    uint8_t ex_id = *((uint8_t *)args);

    shared_data_t *shm_data = (shared_data_t *)shared_mem.data;

    char license[7];
    double bill = 0;

    // Start For Loop
    do {

        // Wait for License
        lplate_sensor_read(&shm_data->exits[ex_id].lplate_sensor, license);

        // Get Value of License Plate
        item_t *find_res = htab_find(&vehicle_table, license);
        if(find_res == NULL)
        {
            continue;
        }
        int license_value = find_res->value;

        // Calculate Bill
        bill = calculate_bill(start_time[license_value]);

        // Add to revenue 
        revenue = revenue + bill;

        // Write to Bill.txt
        write_bill(license, bill);
        
        // Open Gate
        boom_gate_admit_one(&shm_data->exits[ex_id].bgate); 
        
    } while(!quit);

    free(args);

    return NULL;
}

// License Plate Monitor keeps track of vehicles entering on the floor
    // Store a 0 value for cars in the park which can be used to display vehicle,
void *lp_monitor( void *args) {

    uint8_t floor = *((uint8_t *)args);

    shared_data_t *shm_data = (shared_data_t *)shared_mem.data;

    char license[LICENSE_PLATE_LENGTH + 1];
    license[LICENSE_PLATE_LENGTH] = '\0';

    // Start For Loop,

    do {

        // Update License
        lplate_sensor_read(&shm_data->levels[floor].lplate_sensor,license);
        // Get Value of License Plate
        item_t *find_res = htab_find(&vehicle_table, license);
        if(find_res == NULL)
        {
            continue;
        }
        int license_value = find_res->value;

        // Check if vehicle is entering
        if (vehicle_tracker[license_value] == 0) {
            vehicle_counter_floor[floor]++;
            vehicle_tracker[license_value] = floor;
        }
        // If not entering, must be leaving
        else {
            vehicle_counter_floor[floor]--;
            vehicle_tracker[license_value] = 0;
        }

    } while(!quit);

    free(args);

    return NULL;
}

// Function
int main(void)
{
    quit = false;

    // Initialise
            // create a hash table with 10 buckets
    size_t buckets = 10;
    htab_init(&vehicle_table, buckets);
        // Create License Plate Array
    lp_list();

        /* Setup shared memory and attach: */
   shared_mem_data_init(&shared_mem, SHM_SIZE, SHM_NAME, SHM_NAME_LENGTH);
    if(!shared_mem_attach(&shared_mem))
    {
        return -1;
    }
    shared_mem_data_init(&handshake_mem, SHM_LINK_MANAGER_SIZE, SHM_HANDSHAKE_NAME, SHM_HANDSHAKE_NAME_LENGTH);
    if(!shared_mem_attach(&handshake_mem))
    {
        return -1;
    }

        /* Notify the simulator that the manager has successfully attached: */
    shared_handshake_t *handshake_data = (shared_handshake_t *)handshake_mem.data;
    sem_post(&handshake_data->manager_linked);

    /* Create thread for monitoring running state of sim: */
    pthread_t quit_thread;
    pthread_create(&quit_thread, NULL, wait_sim_close, NULL);

    // Create Thread for Entrance
    pthread_t entrance_monitor_thread[NUM_ENTRANCES];
    {
        uint8_t *ids[NUM_ENTRANCES];
        for (int i = 0; i < NUM_ENTRANCES; i++){
            ids[i] = (uint8_t *)malloc(sizeof(int));
            *ids[i] = i;
            pthread_create(&entrance_monitor_thread[i], NULL, entrance_monitor, (void *)ids[i]);
        }
    }

    // Create Thread for Exit
    pthread_t exit_monitor_thread[NUM_EXITS];
    {
        uint8_t *ids[NUM_EXITS];
        for (int i = 0; i < NUM_EXITS; i++){
            ids[i] = (uint8_t *)malloc(sizeof(int));
            *ids[i] = i;
            pthread_create(&exit_monitor_thread[i], NULL, exit_monitor, (void *)ids[i]);
        }
    }

    // Create thread for LP sensor
    
    pthread_t lp_monitor_thread[NUM_LEVELS];
    {
        uint8_t *ids[NUM_LEVELS];
        for (int i = 0; i < NUM_LEVELS; i++){
            ids[i] = (uint8_t *)malloc(sizeof(int));
            *ids[i] = i;
            pthread_create(&lp_monitor_thread[i], NULL, lp_monitor, (void *)ids[i]);
        }
    }

    // Displaying Information

    do {

        system("clear");
        // Signs Display
        printf("Car Park\nCapacity: %d/%d\n", vehicle_counter_total,NUM_LEVELS*FLOOR_CAPACITY);

        for (int i = 0; i < NUM_LEVELS; i++){
            printf("Level: %d \t| License Plate Reader: %s\t| Capacity: %d/%d\n",i + 1, level_lps_current[i],vehicle_counter_floor[i],FLOOR_CAPACITY);
        }
        printf("\n");

        for (int i = 0; i < NUM_ENTRANCES; i++){
            printf("Entrance: %d \t| License Plate Reader: %s\t| Sign\n",i + 1, entrance_lps_current[i]);
        }
        printf("\n");

        for (int i = 0; i < NUM_EXITS; i++){
            printf("Exit: %d \t| License Plate Reader: %s\t| Sign\n",i + 1, exit_lps_current[i]);
        }

        // Display current status of parking

        usleep(50000);

    } while(!quit);

    /* Shutdown sequence: */
        /* Join threads: */
    pthread_join(quit_thread, NULL);
        /* Entrances: */
    for (int i = 0; i < NUM_ENTRANCES; i++){
        pthread_join(entrance_monitor_thread[i], NULL);
    }
        /* Exits: */
    for (int i = 0; i < NUM_EXITS; i++){
        pthread_join(exit_monitor_thread[i], NULL);
    }
        /* LP sensors: */
    for (int i = 0; i < NUM_LEVELS; i++){
        pthread_join(lp_monitor_thread[i], NULL);
    }
}