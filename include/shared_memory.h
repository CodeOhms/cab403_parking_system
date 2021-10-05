
typedef struct shared_mem;

struct license_plate_sensor
{
    pthread_mutex_t ;
    pthread_cond_t ;
    char license_plate[6];
};

enum boom_gate_state
{
    C,
    O,
    R,
    L
};

struct boom_gate
{
    pthread_mutex_t ;
    pthread_cond_t ;
    boom_gate_state bg_state;
};

/*
The information sign is very basic and only has room to display a single character. It is used
to show information to drivers at various points:
● When the driver pulls up in front of the entrance boom gate and triggers the LPR, the
sign will show a character between ‘1’ and ‘5’ to indicate which floor the driver should
park on.
● If the driver is unable to access the car park due to not being in the access file, the
sign will show the character ‘X’.
● If the driver is unable to access the car park due to it being full, the sign will show the
character ‘F’.
● In the case of a fire, the information sign will cycle through the characters ‘E’ ‘V’ ‘A’
‘C’ ‘U’ ‘A’ ‘T’ ‘E’ ‘ ‘, spending 20ms on each character and then looping back to the
first ‘E’ after displaying the space character..
*/


struct information_sign
{
    pthread_mutex_t ;
    pthread_cond_t ;
    char display[1];
};

struct entrance
{
    license_plate_sensor lplate_sensor;
    boom_gate bgate;
    information_sign info_sign;
};

struct shared_mem
{

};