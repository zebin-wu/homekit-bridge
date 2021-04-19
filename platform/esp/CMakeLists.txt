cmake_minimum_required(VERSION 3.5)

set(TOP_DIR ${CMAKE_SOURCE_DIR}/../..)
include(${TOP_DIR}/cmake/project.cmake)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
if(CONFIG_HAP_MFI_HW_AUTH)
    add_definitions(-DHAVE_MFI_HW_AUTH=1)
endif()

project(${PROJECT})
