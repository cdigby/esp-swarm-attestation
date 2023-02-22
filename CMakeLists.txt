# (Automatically converted from project Makefile by convert_to_cmake.py.)

# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/../../esp/esp-privilege-separation/components/protected
    ${CMAKE_CURRENT_LIST_DIR}/../../esp/esp-privilege-separation/components/shared
    ${CMAKE_CURRENT_LIST_DIR}/protected_app
)

set(CUSTOM_SYSCALL_TBL ${CMAKE_CURRENT_LIST_DIR}/components/sa_syscall/sa_syscalls.tbl)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp_swarm_attestation)
