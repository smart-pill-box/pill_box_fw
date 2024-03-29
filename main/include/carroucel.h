#include "continuos_servo.h"

typedef struct CarroucelConfig
{
    int nun_of_positions;
    int motor_gpio;
    int* positions_calibrations; // The size of this has to be two times nun_of_positions
} CarroucelConfig;

typedef enum Direction {
    CCW,
    ACW
} Direction;

typedef struct CarroucelEvent {
    int position;
    Direction direction;
} CarroucelEvent;

// Positions calibrations are the ADC output of the potentiometer for each one of the positions
// but twice, so that we can go to any position in any direction CCW or ACW. For example
// if nun_of_positions = 5
// positions_calibrations = { 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000 }

typedef struct Carroucel {
    int curr_position;
    CarroucelConfig config;
    ServoHandler servo;
    Direction move_direction;
    bool on_position;
} Carroucel;

void setup_carroucel(CarroucelConfig config);

void carroucel_to_pos(int pos);

void carroucel_to_pos_ccw(int pos);

void carroucel_to_pos_acw(int pos);

bool carroucel_is_on_position();

int get_carroucel_pos();

// void carroucel_next_pos();

// void carroucel_back_pos();

