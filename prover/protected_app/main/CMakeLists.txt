idf_component_register(
    SRCS "protected_main.c"
    INCLUDE_DIRS "."
    REQUIRES bootloader_support spi_flash esp_priv_access sa_config sa_network sa_algorithms sa_protected_comms
)
