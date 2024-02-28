#include "driver/mcpwm_prelude.h"

typedef struct {
    int servo_gpio_pin;
    uint32_t pwm_period_us;
    uint32_t reverse_max_period_us;
    uint32_t direct_max_period_us;
    uint32_t break_period_us;
} ServoMotorConfig;

typedef struct {
    ServoMotorConfig config;
    mcpwm_timer_handle_t timer;
    mcpwm_oper_handle_t operator;
    mcpwm_cmpr_handle_t comparator;
    mcpwm_gen_handle_t generator;
} ServoHandler;

ServoMotorConfig init_servo_config();

void new_continuos_servo(ServoMotorConfig motor_config, ServoHandler* servo_handler);

void servo_start(ServoHandler handler);
void servo_break(ServoHandler handler);
void servo_run_percentage(ServoHandler handler, int speed_percentage);
void servo_run_raw(ServoHandler handler, uint32_t period);

uint32_t speed_percentage_to_period(int speed_percentage, ServoMotorConfig config);
