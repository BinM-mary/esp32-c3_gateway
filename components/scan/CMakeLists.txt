idf_component_register(SRCS "scan.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi esp_event nvs_flash json)
