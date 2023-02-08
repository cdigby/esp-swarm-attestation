idf_component_register(
    SRCS "comms.c" "user_code.c"
    INCLUDE_DIRS ".."
    REQUIRES sa_syscall
)
