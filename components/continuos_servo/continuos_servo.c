#include "continuos_servo.h"
#include "esp_log.h"

#define TAG "continuos_servo"

#define TIMER_RESOLUTION_HZ 1000000 // 1us / tick

ServoMotorConfig init_servo_config(){
    ServoMotorConfig config = {
        .servo_gpio_pin = 0,
        .pwm_period_us = 20000,
        .reverse_max_period_us = 1000,
        .direct_max_period_us = 2000,
        .break_period_us = 1500,
    };

    return config;
}

void new_continuos_servo(ServoMotorConfig motor_config, ServoHandler* servo_handler){
    servo_handler->config = motor_config;
    if(motor_config.reverse_max_period_us > motor_config.direct_max_period_us){
        return;
    }

    ESP_LOGI(TAG, "Create timer and operator");
    mcpwm_timer_handle_t timer = NULL;
    mcpwm_timer_config_t timer_config = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = TIMER_RESOLUTION_HZ,
        .period_ticks = motor_config.pwm_period_us,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &timer));
    servo_handler->timer = timer;

    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t operator_config = {
        .group_id = 0, // operator must be in the same group to the timer
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &oper));
    servo_handler->operator = oper;

    ESP_LOGI(TAG, "Connect timer and operator");
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    ESP_LOGI(TAG, "Create comparator and generator from the operator");
    mcpwm_cmpr_handle_t comparator = NULL;
    mcpwm_comparator_config_t comparator_config = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &comparator_config, &comparator));
    servo_handler->comparator = comparator;

    mcpwm_gen_handle_t generator = NULL;
    mcpwm_generator_config_t generator_config = {
        .gen_gpio_num = motor_config.servo_gpio_pin,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &generator_config, &generator));
    servo_handler->generator = generator;

    // set the initial compare value, so that the servo will spin to the center position
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, motor_config.break_period_us));

    ESP_LOGI(TAG, "Set generator action on timer and compare event");
    // go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(generator,
                                                                MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    // go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(generator,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, comparator, MCPWM_GEN_ACTION_LOW)));
}

void servo_start(ServoHandler handler){
    ESP_LOGI(TAG, "Enable and start timer");
    ESP_ERROR_CHECK(mcpwm_timer_enable(handler.timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(handler.timer, MCPWM_TIMER_START_NO_STOP));

    servo_break(handler);
}

void servo_break(ServoHandler handler){
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(handler.comparator, handler.config.break_period_us));
}

void servo_run_percentage(ServoHandler handler, int speed_percentage){
    uint32_t period = speed_percentage_to_period(speed_percentage, handler.config);
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(handler.comparator, period));
}

void servo_run_raw(ServoHandler handler, uint32_t period){
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(handler.comparator, period));
}

uint32_t speed_percentage_to_period(int speed_percentage, ServoMotorConfig config){
    uint32_t percentage;
    if(speed_percentage >= 0){
        percentage = (uint32_t) speed_percentage;
        return config.break_period_us + (config.direct_max_period_us - config.break_period_us)*percentage/100;
    } else {
        percentage = (uint32_t) (-speed_percentage);
        return config.break_period_us - (config.break_period_us - config.reverse_max_period_us)*percentage/100;
    }
}

