#include <esp_http_server.h>
#include <esp_log.h>

#include "carroucel.h"

#define TAG "rest_server.c"
#define NUN_OF_ENDPOINTS 3

static esp_err_t start_reloading(httpd_req_t *req){
    printf("Start reloading");
    httpd_resp_send(req, "Start reloading", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t load_pill(httpd_req_t *req){
    printf("Load pill");
    carroucel_next_pos();
    httpd_resp_send(req, "Start reloading", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t finish_reloading(httpd_req_t *req){
    printf("Finish reloading");
    httpd_resp_send(req, "Finish reloading", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t endpoints[NUN_OF_ENDPOINTS] = {
    {.uri = "/start_reloading", .method = HTTP_POST, .handler = start_reloading},
    {.uri = "/finish_reloading", .method = HTTP_POST, .handler = finish_reloading},
    {.uri = "/load_pill", .method = HTTP_POST, .handler = load_pill},
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

