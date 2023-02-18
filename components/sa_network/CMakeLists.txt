idf_component_register(
    SRCS "sa_network.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_netif nvs_flash esp_event sa_config
)
