# Note: esp_ipc, esp_pm added as a public requirement to keep compatibility as to be located here.
idf_component_register(SRCS "as5600.c"
                        INCLUDE_DIRS "include"
                        REQUIRES driver)
