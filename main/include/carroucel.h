#include "potentiometer.h"
#include "continuos_servo.h"

typedef struct CarroucelConfig
{
    int nun_of_positions;
    int motor_gpio;
    int angle_sensor_gpio;
    int* positions_calibrations; 
} CarroucelConfig;

typedef struct Carroucel {
    int curr_position;
    CarroucelConfig config;
    ServoHandler servo;
    Potentiometer potentiometer;
} Carroucel;

void setup_carroucel(CarroucelConfig config);

void carroucel_to_pos(int pos);

void carroucel_next_pos();

void carroucel_back_pos();

