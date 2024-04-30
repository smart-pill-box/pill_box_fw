#include "carroucel.h"
#include "angle_sensor.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#define SERVO _carroucel.servo
#define CALIBRATIONS _carroucel.config.positions_calibrations

#define ERROR_HYSTERESIS 50
#define MIN_ERROR 50
#define MAX_ERROR 18000
#define ANY_DIRECTION_ERROR 900

#define MIN_VELOCITY 15
#define MAX_VELOCITY 80

Carroucel _carroucel;
static QueueHandle_t carroucel_queue = NULL;

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
    return get_angle();
}

int get_error_acw(int pos){
    int reference = get_reference_value(pos);
    int measured = get_measured_value();
    int error;

    if(reference <= measured) {
        error = measured - reference;
    } else {
        error = 36000 - (reference - measured); 
    }

    return error;
}

int get_error_cw(int pos){
    int reference = get_reference_value(pos);
    int measured = get_measured_value();
    int error;

    if(reference >= measured) {
        error = reference - measured;
    } else {
        error = 36000 - (measured - reference); 
    }

    return error;
}

int get_error(int pos){
    if(_carroucel.move_direction == CCW){
        return get_error_cw(pos);
    }

    return get_error_acw(pos);
}

void carroucel_task(){
    CarroucelEvent event;
    int error = 10000;
    while(true){
        if(xQueueReceive(carroucel_queue, &event, 0)){
            _carroucel.curr_position = event.position;
            _carroucel.move_direction = event.direction;
            _carroucel.on_position = false;
        }

        error = get_error(_carroucel.curr_position);
        int min_error = MIN_ERROR;
        if(_carroucel.on_position){
            min_error = MIN_ERROR + ERROR_HYSTERESIS;
        }

        while(error > min_error && 36000 - error > min_error){
            _carroucel.on_position = false;
            error = get_error(_carroucel.curr_position);
            
            int limited_error = limit_value(error, 0, MAX_ERROR);
            int velocity = map_value(limited_error, 0, MAX_ERROR, MIN_VELOCITY, MAX_VELOCITY);

            if(_carroucel.move_direction == CCW){
                velocity = -velocity;
            }
        
            if(36000 - error <= ANY_DIRECTION_ERROR){
                if(_carroucel.move_direction == CCW){
                    velocity = MIN_VELOCITY;
                } else {
                    velocity = -MIN_VELOCITY;
                }
            }

            servo_run_percentage(SERVO, velocity);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        servo_run_percentage(SERVO, 0);

        _carroucel.on_position = true;
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void start_carroucel_task(){
    carroucel_queue = xQueueCreate(4, sizeof(CarroucelEvent));

    xTaskCreate(carroucel_task, "carroucel_task", 2048, NULL, 2, NULL);
}

void setup_carroucel(CarroucelConfig config){
    setup_angle_sensor();

    ServoMotorConfig servo_config = init_servo_config();
    servo_config.servo_gpio_pin = config.motor_gpio;

    ServoHandler servo;
    new_continuos_servo(servo_config, &servo);

    _carroucel.curr_position = 0;
    _carroucel.config = config;
    _carroucel.servo = servo;
    _carroucel.on_position = false;
    _carroucel.move_direction = ACW;

    servo_start(_carroucel.servo);

    start_carroucel_task();
}

void carroucel_to_pos_ccw(int pos){
    check_position(pos);
    CarroucelEvent event = {
        .position = pos,
        .direction = CCW
    };
    xQueueSend(carroucel_queue, &event, 0);
}

void carroucel_to_pos_acw(int pos){
    check_position(pos);
    CarroucelEvent event = {
        .position = pos,
        .direction = ACW
    };
    xQueueSend(carroucel_queue, &event, 0);
}

bool carroucel_is_on_position(){
    return _carroucel.on_position;
}

int get_carroucel_pos(){
    return _carroucel.curr_position;
}
