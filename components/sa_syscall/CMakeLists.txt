# Adapted from rmaker_switch example

if(USER_APP_BUILD)
    set(srcs sa_wrappers.c)
else()
    set(srcs sa_syscalls.c)
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS include
    PRIV_INCLUDE_DIRS .
    REQUIRES syscall_shared esp_priv_build_utils esp_netif esp_wifi sa_config
)

if(USER_APP_BUILD)
    idf_component_get_property(syscall_wrapper_lib syscall_wrapper COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${syscall_wrapper_lib})
else()
    idf_component_get_property(esp_syscall_lib esp_syscall COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${esp_syscall_lib})

    idf_component_get_property(sa_algorithms_lib sa_algorithms COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${sa_algorithms_lib})

    idf_component_get_property(sa_algorithms_lib sa_network COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${sa_algorithms_lib})
endif()

