#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>

#include "esp_netif_ip_addr.h"
#include "freertos/portmacro.h"
#include "loaded_pills_stack.h"
#include "alarm.h"
#include "constants.h"
#include "pill_box_task.h"
#include "rest_server.h"
#include "wifi_manager.h"
#include "carroucel.h"
#include "api_client.h"
#include "constants.h"

#define TAG "main.c"
#define POTENTIOMETER_PIN 36
#define CONTINUOUS_SERVO_PIN 18
#define NUN_OF_POSITIONS 20

static void on_received_new_ip(
    void* arg, 
    esp_event_base_t event_base,
    int32_t event_id, 
    void* event_data
){
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
	char device_ip[26];
	sprintf(device_ip, IPSTR, IP2STR(&event->ip_info.ip));
	put_device_ip(device_ip);
}

esp_err_t test_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                          uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    if (inbuf) {
        ESP_LOGI(TAG, "Received data: %.*s", inlen, (char *)inbuf);
    }
    char response[] = DEVICE_KEY;
    *outbuf = (uint8_t *)strdup(response);
    if (*outbuf == NULL) {
        ESP_LOGE(TAG, "System out of memory");
        return ESP_ERR_NO_MEM;
    }
    *outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

    return ESP_OK;
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
	add_custom_endpoint("custom-data", test_handler);
    start_wifi_manager_task(config, false);
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_received_new_ip, NULL));

    int* positions_calibrations_p = malloc(sizeof(int)*NUN_OF_POSITIONS);
    int init_position = 273;
    for (int i=0; i < NUN_OF_POSITIONS; i++){
        *(positions_calibrations_p + i) = init_position + 1800*i;
    }

    create_pills_stack(NUN_OF_POSITIONS);
	init_api_client_task();

	CarroucelConfig carroucel_config = {
        .motor_gpio = CONTINUOUS_SERVO_PIN,
        .nun_of_positions = NUN_OF_POSITIONS,
        .positions_calibrations = positions_calibrations_p
    };
    setup_carroucel(carroucel_config);
	alarm_init(23);
    start_pill_box_task();


    bool started = false;
    while (1) {
        if(get_connection_status() == 1 && !started){
            started = true;
            start_webserver();
        }
		printf("End: %d \n Start: %d \n", get_end(), get_start());
		vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}
