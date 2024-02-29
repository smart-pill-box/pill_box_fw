#include "carroucel.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define POTENTIOMETER _carroucel.potentiometer
#define SERVO _carroucel.servo
#define CALIBRATIONS _carroucel.config.positions_calibrations

#define MIN_ERROR 8
#define MAX_ERROR 200

Carroucel _carroucel;

void check_position(int pos){
    if(pos > _carroucel.config.nun_of_positions - 1 || pos < 0){
        // TODO Error
    }
}

int get_next_position(){
    if(_carroucel.curr_position == _carroucel.config.nun_of_positions - 1){
        return 0;
    } else {
        return _carroucel.curr_position + 1;
    }
}

int get_previus_position(){
    if(_carroucel.curr_position == 0){
        return _carroucel.config.nun_of_positions - 1;
    } else {
        return _carroucel.curr_position - 1;
    }
}

int limit_value(int value, int min, int max){
    if (value < min){
        return min;
    } else if(value > max){
        return max;
    } else {
        return value;
    }
}

int map_value(int value, int in_min, int in_max, int out_min, int out_max){
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int get_reference_value(int pos){
    check_position(pos);

    return CALIBRATIONS[pos];
}

int get_measured_value(){
    return get_potentiometer_filtered(POTENTIOMETER);
}

void setup_carroucel(CarroucelConfig config){
    Potentiometer potentiometer;
    init_potentiometer(config.angle_sensor_gpio, &potentiometer);

    ServoMotorConfig servo_config = init_servo_config();
    servo_config.servo_gpio_pin = config.motor_gpio;

    ServoHandler servo;
    new_continuos_servo(servo_config, &servo);

    _carroucel.curr_position = 0;
    _carroucel.config = config;
    _carroucel.servo = servo;
    _carroucel.potentiometer = potentiometer;

    servo_start(_carroucel.servo);
}

void carroucel_to_pos(int pos){
    check_position(pos);

    int error = 100;
    while(error > MIN_ERROR){
        int reference = get_reference_value(pos);
        int measured =  get_measured_value();
        error = reference - measured;

        int limited_error = limit_value(error, -MAX_ERROR, MAX_ERROR);
        int velocity = map_value(limited_error, -MAX_ERROR, MAX_ERROR, -100, 100);

        printf("measured: %d | reference: %d\n", reference, measured);

        servo_run_percentage(SERVO, velocity);
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    _carroucel.curr_position = pos;
}

void carroucel_next_pos(){
    carroucel_to_pos(get_next_position());
}

void carroucel_back_pos(){
    carroucel_to_pos(get_previus_position());
}
