#include "potentiometer.h"
#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define MEDIAN_FILTER_SIZE 12

void init_potentiometer(int gpio_pin, Potentiometer* potentiometer){
    adc_oneshot_unit_handle_t* adc_handle_p = (adc_oneshot_unit_handle_t*) malloc(sizeof(adc_oneshot_unit_handle_t));
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, adc_handle_p));

    adc_channel_t channel = ADC_CHANNEL_0;
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_0,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*adc_handle_p, channel, &config));

    potentiometer->gpio_pin = gpio_pin;
    potentiometer->adc_handle = *adc_handle_p;
    potentiometer->adc_channel = channel;
}

int get_median_from_sorted_list(int arr[], int size){
    if ((size & 1) == 0){
        return (arr[size / 2 - 1] + arr[size / 2]) / 2;
    } else {
        return arr[size / 2];
    }
}

void swap(int* a, int* b){
    int temp = *a;

    *a = *b;
    *b = temp;
}

int get_potentiometer_filtered(Potentiometer handle){
    int measured_values[MEDIAN_FILTER_SIZE];
    int j;

    for (int i = 0; i < MEDIAN_FILTER_SIZE; i++){
        ESP_ERROR_CHECK(adc_oneshot_read(handle.adc_handle, handle.adc_channel, &measured_values[i]));
        vTaskDelay(pdMS_TO_TICKS(5));

        j = i;
        while(j != 0 && measured_values[j] > measured_values[j-1]){
            swap(&measured_values[j], &measured_values[j-1]);
            j--;
        }
    }

    return get_median_from_sorted_list(measured_values, MEDIAN_FILTER_SIZE);
}
