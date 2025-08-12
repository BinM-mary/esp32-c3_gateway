idf_component_register(
    SRCS "mqtt_gateway.c"
    INCLUDE_DIRS "include"
    REQUIRES mqtt protocol_examples_common freertos lwip esp_event esp_netif
)