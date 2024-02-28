
typedef struct wifi_manager_config_t
{
    const char * device_prefix;
    const char * proof_of_possession;
    const char * service_name;
    uint32_t max_number_of_retries;
} wifi_manager_config_t;

void start_wifi_manager_task(wifi_manager_config_t config, bool restart_nvm_wifi_config);

bool get_connection_status();
