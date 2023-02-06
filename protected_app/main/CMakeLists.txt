idf_component_register(
    SRCS "protected_main.c" "network.c"
    INCLUDE_DIRS "."
    REQUIRES bootloader_support spi_flash esp_priv_access esp_wifi esp_netif nvs_flash esp_event
)
