#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

// Create variable
char license_plate[1000][6];

// Setup License plate for reading
void lp_list ( void ) {

    FILE *plate = fopen("plates.txt", "r");

    const unsigned MAX_LENGTH = 256;
    char buffer[6];

    // Counter to assign array
    int counter = 0;

    // Assign Characters to Array
    while (fgets(buffer, MAX_LENGTH, plate)) {
        for (int i = 0; i < 6; i++) {
        license_plate[counter][i] = buffer[i];
        }
        counter++;

    }

    // close the file
    fclose(plate);

}


int main()
{
    // Initialise
        // License Plate List
    lp_list();

    // Testing from list
    int license_no = 1;

    printf("Plate No.%d is : ");
    for (int i = 0; i < 6; i++)
    {
        printf("%c", license_plate[license_no][i]);
    }
    printf("\n");

    // Car Status
        // Monitor Status of LPR Sensor and Keep track of each char

        // Ensure that there is room in car park (Make sure there is less than 20 cars)

        // Keep Track of car parking

        // Update billing.txt when car leaves

    // Boom Gates
        // Tell Boom gate to open

    // Displaying Information
        // Signs Display

        // Display current status of parking 
}