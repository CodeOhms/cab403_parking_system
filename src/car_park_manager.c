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
int license_plate_length = 0;

// Setup License plate for reading
void lp_list ( void ) {

    FILE *plate = fopen("plates.txt", "r");

    const unsigned MAX_LENGTH = 256;
    char buffer[6];

    // Assign Characters to Array
    while (fgets(buffer, MAX_LENGTH, plate)) {

        for (int i = 0; i < 6; i++) {
        license_plate[license_plate_length][i] = buffer[i];
        }
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

int main()
{
    // Initialise

    lp_list();


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



    // TESTING FUNCTIONS //
        // License Plate
            // Testing from list
    int license_no = 1;

    printf("Plate No.%d is : ",license_no + 1);
    for (int i = 0; i < 6; i++)
    {
        printf("%c", license_plate[license_no][i]);
    }
    printf("\n");

    // Scanning for License Plate
    lp_scan("177BLJ");
}