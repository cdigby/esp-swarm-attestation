if(USER_APP_BUILD)
    set(srcs sa_wrappers.c)
else()
    set(srcs sa_syscalls.c)
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
)