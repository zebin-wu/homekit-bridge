# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/system/console/components)

include(${TOP_DIR}/cmake/project.cmake)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(${PROJECT})

if(CONFIG_HAP_MFI_HW_AUTH)
    add_definitions(-DHAVE_MFI_HW_AUTH=1)
endif()
