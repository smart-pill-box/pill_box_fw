#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <string.h>
#include <sys/param.h>
#include <stdlib.h>

#include "api_client.h"
#include "esp_http_client.h"
#include "constants.h"
#include "esp_tls.h"
#include "freertos/portmacro.h"

#define TAG "api_client"
#define MEDICINE_API_HOST "192.168.0.17"
#define MEDICINE_API_PORT 8080
#define MAX_PATH_SIZE 100
#define MAX_HTTP_OUTPUT_BUFFER 1024
#define MAX_POST_DATA_SIZE 255
// Requisições:
// - POST device_pill: /device/<key>/device_pill
// - PUT device_ip: /device/<key>/device_ip
// - PUT device_pill_status: /device/<key>/device_pill/status

typedef struct ApiMessage {
	ApiEndpoint endpoint;
	PostData post_data;
} ApiMessage;

static QueueHandle_t api_queue;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    const int buffer_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(buffer_len);
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (buffer_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

void map_endpoint_to_path(ApiEndpoint endpoint, char path[]){
	switch (endpoint) {
		case POST_DEVICE_IP:
			sprintf(path, "/device/%s/device_ip", DEVICE_KEY);
			break;
	}
}

void api_task(){
	ApiMessage message;
	char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
	while(true){
		if(!xQueueReceive(api_queue, &message, 0)){
			vTaskDelay(200 / portTICK_PERIOD_MS);	
			continue;
        }

		char path[MAX_PATH_SIZE];
		map_endpoint_to_path(message.endpoint, path);
		esp_http_client_config_t config = {
			.host = MEDICINE_API_HOST,
			.port = MEDICINE_API_PORT,
			.path = path,
			.event_handler = _http_event_handler,
			.user_data = local_response_buffer,        // Pass address of local buffer to get response
			.disable_auto_redirect = true,
		};
		esp_http_client_handle_t client = esp_http_client_init(&config);

		char post_data[MAX_POST_DATA_SIZE] = "";
		sprintf(post_data, "{\"device_ip\":\"%s\"}", message.post_data.post_device_ip_data.device_ip);
		free(message.post_data.post_device_ip_data.device_ip);

		char content_length[6] = "";
		sprintf(content_length, "%zu", strlen(post_data));

		esp_http_client_set_method(client, HTTP_METHOD_POST);
		esp_http_client_set_header(client, "Content-Type", "application/json");
		esp_http_client_set_header(client, "Content-Length", content_length);
		esp_http_client_set_post_field(client, post_data, strlen(post_data));

		esp_err_t err = esp_http_client_perform(client);
		if (err == ESP_OK) {
			  ESP_LOGI(TAG,
				   "HTTP POST Status = %d, content_length = "
				   "%" PRIu64,
				   esp_http_client_get_status_code(client),
				   esp_http_client_get_content_length(client));
		} else {
			  ESP_LOGE(TAG, "HTTP POST request failed: %s",
				   esp_err_to_name(err));
		}
	}
}

void init_api_client_task(){
	api_queue = xQueueCreate(2, sizeof(ApiMessage));
	xTaskCreate(api_task, "api_task", 16384, NULL, 2, NULL);
}

int api_make_request(ApiEndpoint endpoint, PostData data){
	return 400;
}

void post_device_ip(char device_ip[]){
	char * device_ip_copy = (char *) malloc(16);
	memcpy(device_ip_copy, device_ip, 16);
	ApiMessage message = {
		.endpoint = POST_DEVICE_IP,
		.post_data = {
			.post_device_ip_data = {
				.device_ip = device_ip_copy
			}
		}
	};
	xQueueSend(api_queue, &message, 0);
}
