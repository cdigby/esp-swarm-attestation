if(USER_APP_BUILD)
    # Component settings when building for user app
    idf_component_register(
        SRCS "sa_wrappers.c"
        INCLUDE_DIRS "include"
    )
else()
    # Component settings when building for protected app
    set(srcs sa_syscalls.c)
    idf_component_register(
        SRCS "sa_syscalls.c"
        INCLUDE_DIRS "include"
        REQUIRES sa_network
    )
endif()

