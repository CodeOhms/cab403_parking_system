#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include "shared_memory.h"
#include "linked_list.h"

// Create variable
char license_plate[1000][7];
int license_plate_length = 0;

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
    printf("hash table with %d buckets\n", h->size);
    for (size_t i = 0; i < h->size; ++i)
    {
        printf("bucket %d: ", i);
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
void lp_list ( htab_t *h ) {

    FILE *plate = fopen("plates.txt", "r");

    const unsigned MAX_LENGTH = 256;
    char buffer[7];

    // Assign Characters to Hash Table
    while (fgets(buffer, MAX_LENGTH, plate)) {

        for (int i = 0; i < 6; i++) {
        license_plate[license_plate_length][i] = buffer[i];
        }
        license_plate[license_plate_length][6] = '\0';

        htab_add(h, license_plate[license_plate_length], license_plate_length+1);

        license_plate_length++;


    }

    // close the file
    fclose(plate);

}

// Function for Scanning For license Plate
int lp_scan(char license[6]){

    for (int i = 0; i < license_plate_length; i++){
        
        // Create Variable for checking
        int check = 1;

        // Check if characters match
        for (int j = 0; j < 6; j++){
            check = check * (license_plate[i][j] == license[j]);
        }

        // Check Variable
        if (check == 1)
        {
            printf("Match Found \n");
            return 1;
        }
        
    }

    printf("No Match\n");
    return 0;
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

int main()
{
    // Initialise
            // create a hash table with 10 buckets
    size_t buckets = 10;
    htab_t h;
    htab_init(&h, buckets);
        // Create License Plate Array
    lp_list( &h );

        /* Setup shared memory and attach: */
    shared_mem_attach(&shared_mem);

    // Car Status
        // Monitor Status of LPR Sensor and Keep track of each char
    printf(shared_mem.data->entrances[0].lplate_sensor.license_plate)
        // Ensure that there is room in car park (Make sure there is less than 20 cars)

        // Keep Track of car parking

        // Update billing.txt when car leaves

    write_bill("ABC123", 12.3);
    write_bill("WOW069", 50.23);
    write_bill("TEST37", 20.85);
    // Boom Gates
        // Tell Boom gate to open

    // Displaying Information
        // Signs Display

        // Display current status of parking 



    // TESTING FUNCTIONS //
        // License Plate
            // Testing from list
    int license_no = 1;

    printf("Plate No.%d is : %s\n",license_no + 1, license_plate[license_no]);

    // Scanning for License Plate
    char licesnse_test[7] = "361ECD";
    if (!htab_find(&h, licesnse_test)){
        printf("Not Plate\n");
    }
    else {
        printf("Found Matching : ");
        item_print(htab_find(&h, licesnse_test));
        printf("\n");
    }


}