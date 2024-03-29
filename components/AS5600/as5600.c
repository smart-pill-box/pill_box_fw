#include "as5600.h"
#include <stdio.h>
#include "driver/gpio.h"

#define MASTER_PORT 0
#define DEVICE_ADDR 0x36
#define AS_5600_PWM_FREQ_HZ 115
#define AS_CLOCK_PERIOD_US 2

volatile uint32_t angle_raw = 0;
volatile uint32_t begin = 0;
volatile uint32_t end = 0;

void write_to_reg(uint8_t reg, uint8_t data);
void read_reg(uint8_t reg, uint8_t* data);
void read_multi_regs(uint8_t reg, uint8_t* data, int nun_of_regs);

void write_to_reg(uint8_t reg, uint8_t data){
    uint8_t write_buffer[2];
    write_buffer[0] = reg;
    write_buffer[1] = data;

    esp_err_t err = i2c_master_write_to_device(MASTER_PORT, DEVICE_ADDR, write_buffer, 2*sizeof(uint8_t), -1);
    
    if (err != ESP_OK){
        printf("Error: %d\n", err);
    }
}

void read_reg(uint8_t reg, uint8_t* data){
    esp_err_t err = i2c_master_write_read_device(
        MASTER_PORT,
        DEVICE_ADDR,
        &reg, 
        sizeof(uint8_t),
        data,
        sizeof(uint8_t),
        -1
    );

    if (err != ESP_OK){
        printf("Error: %d\n", err);
    }
}

void read_multi_regs(uint8_t reg, uint8_t* data, int nun_of_regs){
    ESP_ERROR_CHECK(i2c_master_write_read_device(
        MASTER_PORT,
        DEVICE_ADDR,
        &reg, 
        sizeof(uint8_t),
        data,
        nun_of_regs*sizeof(uint8_t),
        -1
    ));
}

void setup_as5600(int scl_pin, int sda_pin){
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = true,
        .scl_pullup_en = true,
        .master.clk_speed = 400000,
    };

    i2c_param_config(MASTER_PORT, &conf);

    i2c_driver_install(MASTER_PORT, conf.mode, 0, 0, 0);
}

uint32_t as5600_read_raw(){
    uint8_t data[2];
    read_multi_regs(REG_RAW_ANGLE_h, data, 2);

    uint32_t raw_angle = (data[0] << 8) + data[1];

    return raw_angle;
}

int as5600_read_angle(){
    uint8_t data[2];
    read_multi_regs(REG_ANGLE_h, data, 2);

    int angle = (data[0] << 8) + data[1];

    return angle;
}

bool magnet_detected(){
    uint8_t data;
    read_reg(REG_STATUS, &data);
    if(data & STATUS_MD_bits){
        return true;
    }

    return false;
}

int get_as5600_status(){
    uint8_t data;
    read_reg(REG_STATUS, &data);
    if(data & STATUS_ML_bits){
        return 3;
    } else if (data & STATUS_MH_bits){
        return 2;
    } else if(data & STATUS_MD_bits){
        return 1;
    }

    return 0;
}
