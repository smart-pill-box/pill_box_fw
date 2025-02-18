#include <stdio.h>
#include <string.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>

#include <esp_sntp.h>
#include <esp_netif_sntp.h>

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include "wifi_manager.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_DISCONNECTED_BIT BIT1
#define DEVICE_KEY "my_device_key"

static EventGroupHandle_t wifi_event_group;

typedef struct wifi_manager_t {
    wifi_manager_config_t config;
    bool is_connected;
    bool someone_connected_to_protocomm;
} wifi_manager_t;

static wifi_manager_t wifi_manager;

const char * TAG = "wifi_manager";

bool is_to_create_custom_endpoint = false;
char * custom_endpoint_name = NULL;
protocomm_req_handler_t custom_endpoint_handler = NULL;

/* Event handler for catching system events */
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    static int retries;
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                         "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                esp_wifi_connect();
                retries++;
                if (retries >= wifi_manager.config.max_number_of_retries) {
                    ESP_LOGI(TAG, "Failed to connect with provisioned AP, reseting provisioned credentials");
                    wifi_prov_mgr_reset_sm_state_on_failure();
                    retries = 0;
                }

                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                retries = 0;
                break;
            case WIFI_PROV_END:
                wifi_prov_mgr_deinit();
                break;
            default:
                break;
        }
    } else if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "Wifi station started");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
                xEventGroupSetBits(wifi_event_group, WIFI_DISCONNECTED_BIT);
                if (!wifi_manager.someone_connected_to_protocomm){
                    esp_wifi_connect();
                }
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
		esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
		esp_netif_sntp_init(&config);
        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == PROTOCOMM_TRANSPORT_BLE_EVENT) {
        switch (event_id) {
            case PROTOCOMM_TRANSPORT_BLE_CONNECTED:
                ESP_ERROR_CHECK(esp_wifi_disconnect());
                ESP_LOGI(TAG, "BLE transport: Connected");
                break;
            case PROTOCOMM_TRANSPORT_BLE_DISCONNECTED:
                ESP_LOGI(TAG, "BLE transport: Disconnected!");
                break;
            default:
                break;
        }
    } 
}

static void start_provisioning(){
    wifi_prov_mgr_disable_auto_stop(100);
    wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

    const char *pop = wifi_manager.config.proof_of_possession;

    wifi_prov_security1_params_t *sec_params = pop;

    uint8_t custom_service_uuid[] = {
        /* LSB <---------------------------------------
            * ---------------------------------------> MSB */
        0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
        0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02,
    };

    ESP_ERROR_CHECK(wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid));

    if (is_to_create_custom_endpoint) {
      ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_create(custom_endpoint_name));
    }
    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(
        security, (const void *)sec_params, wifi_manager.config.service_name,
        NULL));
    if (is_to_create_custom_endpoint) {
      ESP_ERROR_CHECK(wifi_prov_mgr_endpoint_register(
          custom_endpoint_name, custom_endpoint_handler, NULL));
    }
}

static EventBits_t wait_wifi_connection_or_disconnection(void){
    bool connected_or_disconnected = false;
    EventBits_t bits;

    while(!connected_or_disconnected){
        bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );
        connected_or_disconnected = (bits & WIFI_DISCONNECTED_BIT) | (bits & WIFI_CONNECTED_BIT);
    }
    return bits;
}

static EventBits_t wait_wifi_disconnection(void){
    bool disconnected = false;
    EventBits_t bits;

    while(!disconnected){
        bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_DISCONNECTED_BIT,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );
        disconnected = bits & WIFI_DISCONNECTED_BIT;
    }
    return bits;
}

static EventBits_t wait_wifi_connection(void){
    bool connected = false;
    EventBits_t bits;

    while(!connected){
        bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );
        connected = bits & WIFI_CONNECTED_BIT;
    }

    return bits;
}

static void wifi_manager_task(){
    while (1)
    {
        bool provisioned;
        ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

        /* Configuration for the provisioning manager */
        wifi_prov_mgr_config_t prov_config = {
            .scheme = wifi_prov_scheme_ble,
            .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
        };

        if (provisioned){
            printf("I'M ALREADY PROVISIONED");
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            printf("I'M ALREADY PROVISIONED");
            ESP_ERROR_CHECK(esp_wifi_start());
            printf("I'M ALREADY PROVISIONED");

            uint16_t retries = 0;
            while(1){
                printf("WAITING FOR CONNECTION OR DISCONNECTION");
                EventBits_t bits = wait_wifi_connection_or_disconnection();

                if (bits & WIFI_CONNECTED_BIT){
                    break;
                }

                if (++retries == wifi_manager.config.max_number_of_retries){
                    wifi_config_t wifi_cfg_old;
                    esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg_old);

                    ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_config));
                    start_provisioning();

                    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
                    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg_old));
                    ESP_ERROR_CHECK(esp_wifi_connect());

                    wait_wifi_connection();
                    wifi_prov_mgr_deinit();
                }
            }
        } else {
            printf("I'm not provisioned\n");
            ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_config));
            printf("I'm not provisioned\n");
            start_provisioning();
            

            printf("Waiting connection...\n");
            wait_wifi_connection();
            printf("Conected\n");
        }

        wifi_manager.someone_connected_to_protocomm = false;
        wifi_manager.is_connected = true;
        printf("Waiting disconnection\n");
        wait_wifi_disconnection();
        printf("Disconnected\n");
        wifi_manager.is_connected = false;

        // Esta provisionado ?
            // Se sim se conecta e espera conectar ou se desconectar
            // Se desconectar 5 vezes salva a configuração antiga, volta a provisionar e espera se conectar
        
            // Se não começa a provisionar e espera se conectar

        // espera se desconectar
    }
}

void add_custom_endpoint(char * endpoint_name, protocomm_req_handler_t handler){
	custom_endpoint_name = endpoint_name;
	custom_endpoint_handler = handler;

	is_to_create_custom_endpoint = true;
}

void start_wifi_manager_task(wifi_manager_config_t config, bool restart_nvm_wifi_config){
    wifi_manager.config = config;
    wifi_manager.is_connected = false;
    wifi_manager.someone_connected_to_protocomm = false;

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_event_group = xEventGroupCreate();

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    /* Initialize Wi-Fi including netif with default config */
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    if (restart_nvm_wifi_config){
        ESP_ERROR_CHECK(wifi_prov_mgr_reset_provisioning());
    }

    xTaskCreate(wifi_manager_task, "wifi_manager_task", 10240, (void *) &config, 10, NULL);
}

bool get_connection_status(){
    return wifi_manager.is_connected;
}

