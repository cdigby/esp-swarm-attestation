if(USER_APP_BUILD)
    idf_component_register(
        SRCS sa_wrappers.c
        INCLUDE_DIRS include
        REQUIRES syscall_wrapper
    )
else()
    set(srcs sa_syscalls.c)
    idf_component_register(
        SRCS sa_syscalls.c
        INCLUDE_DIRS include
    )
endif()