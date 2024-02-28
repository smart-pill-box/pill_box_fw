#include "esp_adc/adc_oneshot.h"

typedef struct {
    int gpio_pin;
    adc_oneshot_unit_handle_t adc_handle;
    adc_channel_t adc_channel;
} Potentiometer;

void init_potentiometer(int gpio_pin, Potentiometer* potentiometer);

int get_potentiometer_filtered(Potentiometer handle);