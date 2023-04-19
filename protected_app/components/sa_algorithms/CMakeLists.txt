idf_component_register(
    SRCS "sa_simple_plus.c" "sa_shared.c" "sa_simple.c"
    INCLUDE_DIRS "include"
    REQUIRES hacl-star sa_protected_comms
)
