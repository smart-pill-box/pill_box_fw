#include "driver/i2c.h"

#define REG_ZPOS_h 0x01 // start position in degrees 
#define REG_ZPOS_l 0x02 // start position in degrees 
#define REG_MPOS_h 0x03 // stop position in degrees
#define REG_MPOS_l 0x04 // stop position in degrees
#define REG_MANG_h 0x05 // len of position in degrees, set that or the MPOS, but not both
#define REG_MANG_l 0x06 // len of position in degrees, set that or the MPOS, but not both

#define REG_CONF_h 0x07
#define CONF_h_WD_bits  0b00100000 // Watchdog
#define CONF_h_FTH_bits 0b00011100 // Fast filter
#define CONF_h_SF_bits  0b00000011 // Slow filter

#define REG_CONF_l 0x08
// PWM frequency, the PWM pulse contains 128 clock periods (cp) low, then 2095 clock periods
// representing the data, and then 128 cp low again
// 00 -> 115Hz, cp = 2us
// 01 -> 230Hz, cp = 1us
// 10 -> 460Hz, cp = 0.5us
// 11 -> 920Hz, cp = 0.25us
#define CONF_l_PWMF_bits 0b11000000 
#define CONF_l_OUTS_bits 0b00110000 // 00 -> analog out, 01 -> reduced analog out, 10 -> digital PWM out
#define CONF_l_HYST_bits 0b00001100 // Hysteresis
#define CONF_l_PM_bits   0b00000011 // Power mode

#define REG_STATUS 0x0b
#define STATUS_MD_bits 0b00100000 // Magnet detected bit
#define STATUS_ML_bits 0b00010000 // Maximum gain overflow, magnet too week
#define STATUS_MH_bits 0b00001000 // Minimum gain overflow, magnet too strong

#define REG_RAW_ANGLE_h 0x0c
#define REG_RAW_ANGLE_l 0x0d

#define REG_ANGLE_h 0x0e
#define REG_ANGLE_l 0x0f


typedef struct As5600 {
    int pwm_out_pin;
    int i2c_scl_pin;
    int i2c_sda_pin;
} As5600;

void setup_as5600(int scl_pin, int sda_pin);

uint32_t as5600_read_raw();
int as5600_read_angle();

bool magnet_detected();

int get_as5600_status();

