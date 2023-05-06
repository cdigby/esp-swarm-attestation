idf_component_register(
    SRCS "sa_simple_plus.c" "sa_shared.c" "sa_simple.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_common hacl_star sa_protected_comms sa_config
)
