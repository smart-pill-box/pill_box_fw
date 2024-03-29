#include <stdio.h>
#include <string.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>

#include "rest_server.h"
#include "wifi_manager.h"
#include "carroucel.h"

#define TAG "main.c"
#define POTENTIOMETER_PIN 36
#define CONTINUOUS_SERVO_PIN 18

static void on_received_new_ip(
    void* arg, 
    esp_event_base_t event_base,
    int32_t event_id, 
    void* event_data
){
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_manager_config_t config = {
        .device_prefix = "ESP_PROV_",
        .max_number_of_retries = 3,
        .proof_of_possession = "pop",
        .service_name = "MY_BOX"
    };
    start_wifi_manager_task(config, false);
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_received_new_ip, NULL));

    int* positions_calibrations_p = malloc(sizeof(int)*8);
    int positions_calibrations[8] = {2260, 2332, 2390, 2467, 2533, 2607, 2680, 2750};

    memcpy(positions_calibrations_p, positions_calibrations, sizeof(int)*8);

    CarroucelConfig carroucel_config = {
        .angle_sensor_gpio = 36,
        .motor_gpio = CONTINUOUS_SERVO_PIN,
        .nun_of_positions = 8,
        .positions_calibrations = positions_calibrations
    };
    setup_carroucel(carroucel_config);

    bool started = false;
    while (1) {
        printf("Connection status is %d \n", get_connection_status());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if(get_connection_status() == 1 && !started){
            started = true;
            start_webserver();
        }
    }

}
