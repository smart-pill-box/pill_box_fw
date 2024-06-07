#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <time.h>

#include "api_client.h"
#include "esp_http_client.h"
#include "constants.h"
#include "esp_tls.h"
#include "esp_wifi.h"
#include "freertos/portmacro.h"
#include "lwip/err.h"

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
char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

esp_err_t on_put_device_ip(PutDeviceIpData * post_data, esp_http_client_handle_t * client);
esp_err_t on_post_device_pill(PostDevicePillData * post_data, esp_http_client_handle_t * client);
esp_err_t on_put_device_pill_state(PutDevicePillStateData * put_data, esp_http_client_handle_t * client);

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

void map_endpoint_to_path(ApiMessage * message, char path[]){
	switch (message->endpoint) {
		case PUT_DEVICE_IP:
			sprintf(path, "/device/%s/device_ip", DEVICE_KEY);
			break;
		case POST_DEVICE_PILL:
			sprintf(path, "/device/%s/device_pill", DEVICE_KEY);
			break;
		case PUT_DEVICE_PILL_STATUS:
			sprintf(path, "/device/%s/device_pill/%s/status", DEVICE_KEY, message->post_data.put_device_pill_state_data.pill_key);
			free(message->post_data.put_device_pill_state_data.pill_key);
			break;
	}
}

void api_task(){
	ApiMessage message;
	while(true){
		if(!xQueueReceive(api_queue, &message, 0)){
			vTaskDelay(200 / portTICK_PERIOD_MS);	
			continue;
        }

		char path[MAX_PATH_SIZE];
		map_endpoint_to_path(&message, path);
		esp_http_client_config_t config = {
			.host = MEDICINE_API_HOST,
			.port = MEDICINE_API_PORT,
			.path = path,
			.event_handler = _http_event_handler,
			.user_data = local_response_buffer,        // Pass address of local buffer to get response
			.disable_auto_redirect = true,
		};

		esp_http_client_handle_t client = esp_http_client_init(&config);

		esp_err_t err = ESP_OK;
		switch (message.endpoint) {
			case PUT_DEVICE_IP:
				err = on_put_device_ip(&message.post_data.put_device_ip_data, &client);
				break;
			case POST_DEVICE_PILL:
				err = on_post_device_pill(&message.post_data.post_device_pill_data, &client);
				break;
			case PUT_DEVICE_PILL_STATUS:
				err = on_put_device_pill_state(&message.post_data.put_device_pill_state_data, &client);
		}

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

		esp_http_client_cleanup(client);
	}
}

void init_api_client_task(){
	api_queue = xQueueCreate(2, sizeof(ApiMessage));
	xTaskCreate(api_task, "api_task", 16384, NULL, 2, NULL);
}

int api_make_request(ApiEndpoint endpoint, PostData data){
	return 400;
}

void put_device_ip(char device_ip[]){
	char * device_ip_copy = (char *) malloc(17);
	strcpy(device_ip_copy, device_ip);
	ApiMessage message = {
		.endpoint = PUT_DEVICE_IP,
		.post_data = {
			.put_device_ip_data = {
				.device_ip = device_ip_copy
			}
		}
	};
	xQueueSend(api_queue, &message, 0);
}

void post_device_pill(int position, Pill * pill){
	Pill * pill_copy = malloc(sizeof(Pill));
	char * pill_key_copy = malloc(sizeof(char) * 37);

	memcpy(pill_copy, pill, sizeof(Pill));
	memcpy(pill_key_copy, pill->pill_key, sizeof(char) * 37);

	pill_copy->pill_key = pill_key_copy;

	ApiMessage message = {
		.endpoint = POST_DEVICE_PILL,
		.post_data = {
			.post_device_pill_data = {
				.position = position,
				.pill = pill_copy
			}
		}
	};
	xQueueSend(api_queue, &message, 0);
}

esp_err_t on_put_device_ip(PutDeviceIpData * post_data, esp_http_client_handle_t * client){
	char request_body[MAX_POST_DATA_SIZE] = "";
	sprintf(request_body, "{\"deviceIp\":\"%s\"}", post_data->device_ip);
	free(post_data->device_ip);

	char content_length[6] = "";
	sprintf(content_length, "%zu", strlen(request_body));

	esp_http_client_set_method(*client, HTTP_METHOD_PUT);
	esp_http_client_set_header(*client, "Content-Type", "application/json");
	esp_http_client_set_header(*client, "Content-Length", content_length);
	esp_http_client_set_post_field(*client, request_body, strlen(request_body));

	return esp_http_client_perform(*client);
}

esp_err_t on_post_device_pill(PostDevicePillData * post_data, esp_http_client_handle_t * client){
	char request_body[MAX_POST_DATA_SIZE] = "";
	
	char pill_datetime[21];
	struct tm tm;
	
	printf("pill Key is: %s", post_data->pill->pill_key);
	gmtime_r(&post_data->pill->pill_datetime, &tm);
	strftime(pill_datetime, 21, "%Y-%m-%dT%H:%M:%SZ", &tm);
	sprintf(
		request_body, 
		"{\"position\":%d,\"pillDatetime\":\"%s\",\"devicePillKey\":\"%s\"}", 
		post_data->position,
		pill_datetime,
		post_data->pill->pill_key
	);
	free(post_data->pill->pill_key);
	free(post_data->pill);

	char content_length[6] = "";
	sprintf(content_length, "%zu", strlen(request_body));

	printf("\n\n POST DEVICE PILL %s", request_body);
	printf("Content length is %s", content_length);

	esp_http_client_set_method(*client, HTTP_METHOD_POST);
	esp_http_client_set_header(*client, "Content-Type", "application/json");
	esp_http_client_set_header(*client, "Content-Length", content_length);
	esp_http_client_set_post_field(*client, request_body, strlen(request_body));

	return esp_http_client_perform(*client);
}

esp_err_t on_put_device_pill_state(PutDevicePillStateData * put_data, esp_http_client_handle_t * client){
	char request_body[MAX_POST_DATA_SIZE] = "";
	char status_str[50];

	switch (put_data->state) {
		case PILL_BOX_CONFIRMED:
			sprintf(status_str, "%s", "pillBoxConfirmed");
			break;
	}

	sprintf(
		request_body, 
		"\"status\":\"%s\"", 
		status_str
	);

	char content_length[6] = "";
	sprintf(content_length, "%zu", strlen(request_body));

	esp_http_client_set_method(*client, HTTP_METHOD_POST);
	esp_http_client_set_header(*client, "Content-Type", "application/json");
	esp_http_client_set_header(*client, "Content-Length", content_length);
	esp_http_client_set_post_field(*client, request_body, strlen(request_body));

	return esp_http_client_perform(*client);
}

