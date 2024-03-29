#include "as5600.h"

#define SCL_PIN 22
#define SDA_PIN 21

As5600 as_5600;

int get_angle(){
    int raw_angle = as5600_read_raw(as_5600);

    return (36000 * raw_angle) / 4085;
}

void setup_angle_sensor(){
    setup_as5600(SCL_PIN, SDA_PIN);
}