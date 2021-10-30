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
#include "thread_pool.h"
#include "manage_hardware.h"

#define FLOOR_CAPACITY 20
#define NUM_LEVELS 5

// Create variable
char license_plate[100][7];
int vehicle_tracker[100];
int license_plate_length = 0;
double start_time[100];

int revenue = 0;
int vehicle_counter_floor[NUM_LEVELS];
int vehicle_counter_total;

shared_mem_t shared_mem;

// Hash Table
typedef struct item item_t;
struct item {
    char *key;
    int value;
    item_t *next;
};

void item_print(item_t *i) {
    printf("key=%s value=%d", i->key, i->value);
}

    // A hash table mapping a string to an integer.
typedef struct htab htab_t;
struct htab {
    item_t **buckets;
    size_t size;
};

htab_t vehicle_table;

    // Initialise a new hash table with n buckets.
bool htab_init(htab_t *h, size_t n) {
    h->size = n;
    h->buckets = (item_t **)calloc(n, sizeof(item_t *));
    return h->buckets != 0;
}

    // The Bernstein hash function.
size_t djb_hash(char *s) {
    size_t hash = 5381;
    int c;
    while ((c = *s++) != '\0')
    {
        // hash = hash * 33 + c
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

    // Calculate the offset for the bucket for key in hash table.
size_t htab_index(htab_t *h, char *key) {
    return djb_hash(key) % h->size;
}

    // Find pointer to head of list for key in hash table.
item_t *htab_bucket(htab_t *h, char *key) {
    return h->buckets[htab_index(h, key)];
}

    // Find an item for key in hash table.
item_t *htab_find(htab_t *h, char *key) {
    for (item_t *i = htab_bucket(h, key); i != NULL; i = i->next)
    {
        if (strcmp(i->key, key) == 0)
        { // found the key
            return i;
        }
    }
    return NULL;
}

    // Add a key with value to the hash table.
bool htab_add(htab_t *h, char *key, int value) {
    // allocate new item
    item_t *newhead = (item_t *)malloc(sizeof(item_t));
    if (newhead == NULL)
    {
        return false;
    }

    newhead->key = key;
    newhead->value = value;

    // hash key and place item in appropriate bucket
    size_t bucket = htab_index(h, key);
    newhead->next = h->buckets[bucket];
    h->buckets[bucket] = newhead;
    return true;
}

    // Print the hash table.
void htab_print(htab_t *h) {
    printf("hash table with %ld buckets\n", h->size);
    for (size_t i = 0; i < h->size; ++i)
    {
        printf("bucket %ld: ", i);
        if (h->buckets[i] == NULL)
        {
            printf("empty\n");
        }
        else
        {
            for (item_t *j = h->buckets[i]; j != NULL; j = j->next)
            {
                item_print(j);
                if (j->next != NULL)
                {
                    printf(" -> ");
                }
            }
            printf("\n");
        }
    }
}

    // Delete an item with key from the hash table.
void htab_delete(htab_t *h, char *key) {
    item_t *head = htab_bucket(h, key);
    item_t *current = head;
    item_t *previous = NULL;
    while (current != NULL)
    {
        if (strcmp(current->key, key) == 0)
        {
            if (previous == NULL)
            { // first item in list
                h->buckets[htab_index(h, key)] = current->next;
            }
            else
            {
                previous->next = current->next;
            }
            free(current);
            break;
        }
        previous = current;
        current = current->next;
    }
}

    // Destroy an initialised hash table.
void htab_destroy(htab_t *h) {
    // free linked lists
    for (size_t i = 0; i < h->size; ++i)
    {
        item_t *bucket = h->buckets[i];
        while (bucket != NULL)
        {
            item_t *next = bucket->next;
            free(bucket);
            bucket = next;
        }
    }

    // free buckets array
    free(h->buckets);
    h->buckets = NULL;
    h->size = 0;
}

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

    int *gate = (int *)args;

    printf("Boom Gate Entrance Created\n");
    
    char license[7];
    int floor_signal;

    struct timeval time;

    // Start For Loop
    for(;;){
        
        // Wait for License Plate
        lplate_sensor_read(&shared_mem.data->entrances[gate].lplate_sensor,license);
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
            // Acquire Mutex

            // Unlock Mutex

            // Update

            }
        }
    }
}

// Function to open Exit Boom Gate
void *exit_monitor(void *args) {

    int *gate = (int *)args;
    printf("Boomgate Exit Created\n");

    char license[7];
    double bill = 0;

    // Start For Loop
    for(;;){

        // Wait for License
        lplate_sensor_read(&shared_mem.data->exits[gate].lplate_sensor,license);

        // Get Value of License Plate
        int license_value = htab_find(&vehicle_table, license)->value;

        // Calculate Bill
        bill = calculate_bill(start_time[license_value]);

        // Add to revenue 
        revenue = revenue + bill;

        // Write to Bill.txt
        write_bill(license, bill);
        
        // Open Gate
            // Acquire Mutex

            // Unlock Mutex

            // Update


    }

}

// License Plate Monitor keeps track of vehicles entering on the floor
    // Store a 0 value for cars in the park which can be used to display vehicle,
void *lp_monitor( void *args) {

    int *floor = (int *)args;
    printf("License Plate Sensor Created \n");

    char license[7];

    // Start For Loop, license)->value;

    for(;;){

        // Update License
        lplate_sensor_read(&shared_mem.data->levels[floor].lplate_sensor,license);

        // Get Value of License Plate
        int license_value = htab_find(&vehicle_table, license)->value;

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

    }

}

// Function
int main(void)
{
    // Initialise
            // create a hash table with 10 buckets
    size_t buckets = 10;
    htab_init(&vehicle_table, buckets);
        // Create License Plate Array
    lp_list();

        /* Setup shared memory and attach: */
    shared_mem_attach(&shared_mem);
    //shared_mem_attach(&shared_mem);

    // Create Thread for Entrance   
    for (int i = 0; i < NUM_ENTRANCES; i++){
        pthread_t entrance_monitor_thread;
        pthread_create(&entrance_monitor_thread, NULL, entrance_monitor, (void *)&i);
    }

    // Create Thread for Exit
    for (int i = 0; i < NUM_EXITS; i++){
        pthread_t exit_monitor_thread;
        pthread_create(&exit_monitor_thread, NULL, exit_monitor, (void *)&i);
    }

    // Create thread for LP sensor
    for (int i = 0; i < NUM_LEVELS; i++){
        pthread_t lp_monitor_thread;
        pthread_create(&lp_monitor_thread, NULL, lp_monitor, (void *)&i);
    }
    // Displaying Information
        // Signs Display

        // Display current status of parking 


}