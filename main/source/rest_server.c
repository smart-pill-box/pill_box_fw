#include <esp_http_server.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <time.h>

#include "cJSON.h"
#include "carroucel.h"
#include "esp_err.h"
#include "pill_box_task.h"

#define TAG "rest_server.c"
#define NUN_OF_ENDPOINTS 7

#define MAX_POST_SIZE 1024

cJSON* parse_json_or_fail(char * buffer, int buff_size, httpd_req_t * req);
char * parse_string_or_fail(httpd_req_t *req, cJSON* root, char* item_str);

cJSON* parse_json_or_fail(char * buffer, int buff_size, httpd_req_t * req){
    int total_len = req->content_len;
    int cur_len = 0;
    int received = 0;
    if (total_len >= buff_size) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buffer + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
        }
        cur_len += received;
    }
    buffer[total_len] = '\0';
    
    cJSON *root = cJSON_Parse(buffer);
    if(!cJSON_IsObject(root)){
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "The body must be a json");
    }

    return root;
}

char * parse_string_or_fail(httpd_req_t *req, cJSON* root, char* item_str){
    if (!cJSON_HasObjectItem(root, item_str)){
        char error[50];
        sprintf(error, "The parameter %s is required", item_str);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "The item");
    }

    return cJSON_GetObjectItem(root, item_str)->valuestring;
}

static esp_err_t start_reloading(httpd_req_t *req){
	ESP_LOGI(TAG, "\nRECEIVED_REQUEST: start_reloading");
    PillBoxTaskMessageResponse response = send_pill_box_event_sync(START_RELOAD, xTaskGetCurrentTaskHandle());
    if(response == SUCCESS){
        httpd_resp_send(req, "Success", HTTPD_RESP_USE_STRLEN);
    } else if (response == TIMEOUT){
        httpd_resp_send(req, "Erro interno", HTTPD_RESP_USE_STRLEN);
    } else if (response == FAILED) {
        httpd_resp_send(req, "Impossivel recarregar agora", HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send(req, "I don't know", HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}

static esp_err_t take_pill(httpd_req_t *req){
	ESP_LOGI(TAG, "\nRECEIVED_REQUEST: take_pill");
    PillBoxTaskMessageResponse response = send_pill_box_event_sync(TOOK_PILL, xTaskGetCurrentTaskHandle());
    if(response == SUCCESS){
        httpd_resp_send(req, "Success", HTTPD_RESP_USE_STRLEN);
    } else if (response == TIMEOUT){
        httpd_resp_send(req, "Erro interno", HTTPD_RESP_USE_STRLEN);
    } else if (response == FAILED) {
        httpd_resp_send(req, "Impossivel recarregar agora", HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send(req, "I don't know", HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}

static esp_err_t get_status(httpd_req_t *req){
	ESP_LOGI(TAG, "\nRECEIVED REQUEST: get_status");

	PillBoxState state = get_pill_box_state();

	char state_str[50];
	switch (state) {
		case IDLE:
			sprintf(state_str, "idle");
		case PILL_TIME:
			sprintf(state_str, "pill_time");
		case RELOADING:
			sprintf(state_str, "reloading");
		default:
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Internal Error");
	}

	char response_str[100];
	sprintf(response_str, "{'state': %s}", state_str);
	httpd_resp_send(req, response_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t will_load_pill(httpd_req_t *req){
	ESP_LOGI(TAG, "\n RECEIVED_REQUEST: will_load_pill");
    char buf[MAX_POST_SIZE];
    cJSON* root = parse_json_or_fail(buf, MAX_POST_SIZE, req);

    char* pill_key = parse_string_or_fail(req, root, "pill_key");
    char* pill_datetime = parse_string_or_fail(req, root, "pill_datetime");

    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    strptime(pill_datetime, "%Y-%m-%dT%H:%M:%SZ", &tm);

    WillLoadPillMessage message = {
        .pill_to_load = {
            .pill_key = pill_key,
            .pill_datetime = mktime(&tm),
        },
    };
    PillBoxTaskMessageResponse response = send_pill_box_message_sync(WILL_LOAD_PILL, (PillBoxMessage) message, xTaskGetCurrentTaskHandle());
	if(response == SUCCESS){
        httpd_resp_send(req, "Success", HTTPD_RESP_USE_STRLEN);
    } else if (response == TIMEOUT){
        httpd_resp_send(req, "Timeout", HTTPD_RESP_USE_STRLEN);
    } else if (response == FAILED) {
        httpd_resp_send(req, "Failed", HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send(req, "I don't know", HTTPD_RESP_USE_STRLEN);
    }
    httpd_resp_send(req, "Start reloading", HTTPD_RESP_USE_STRLEN);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t load_pill(httpd_req_t *req){
	ESP_LOGI(TAG, "\nRECEIVED_REQUEST: load_pill");
    PillBoxTaskMessageResponse response = send_pill_box_event_sync(LOAD_PILL, xTaskGetCurrentTaskHandle());
    if(response == SUCCESS){
        httpd_resp_send(req, "Success", HTTPD_RESP_USE_STRLEN);
    } else if (response == TIMEOUT){
        httpd_resp_send(req, "Timeout", HTTPD_RESP_USE_STRLEN);
    } else if (response == FAILED) {
        httpd_resp_send(req, "Failed", HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send(req, "I don't know", HTTPD_RESP_USE_STRLEN);
    }
    httpd_resp_send(req, "Start reloading", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t finish_reloading(httpd_req_t *req){
	ESP_LOGI(TAG, "\nRECEIVED_REQUEST: finish_reloading");
    httpd_resp_send(req, "Finish reloading", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t test_connection(httpd_req_t *req){
	httpd_resp_send(req, "Ok", HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

static const httpd_uri_t endpoints[NUN_OF_ENDPOINTS] = {
    {.uri = "/start_reloading", .method = HTTP_POST, .handler = start_reloading},
    {.uri = "/finish_reloading", .method = HTTP_POST, .handler = finish_reloading},
    {.uri = "/load_pill", .method = HTTP_POST, .handler = load_pill},
    {.uri = "/will_load_pill", .method = HTTP_POST, .handler = will_load_pill},
	{.uri = "/test_connection", .method = HTTP_GET, .handler = test_connection},
	{.uri = "/status", .method = HTTP_GET, .handler = get_status},
	{.uri = "/take_pill", .method = HTTP_POST, .handler = take_pill},
};

void start_webserver(){
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if(httpd_start(&server, &config) == ESP_OK){
        for (int i = 0; i < NUN_OF_ENDPOINTS; i++){
            httpd_register_uri_handler(server, &endpoints[i]);
        }
    } else {
        ESP_LOGI(TAG, "Failed starting webserver");
    }
}

