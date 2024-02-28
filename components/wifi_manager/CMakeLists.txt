idf_component_register(SRCS "wifi_manager.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES wifi_provisioning esp_wifi esp_event)
