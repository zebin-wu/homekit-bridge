# Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

# required IDF version, some modules depend on specific IDF version
set(REQUIRED_IDF_VERSION 5.1.0)

# directories
set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(PLUGINS_DIR ${TOP_DIR}/plugins)
set(SCRIPTS_DST_DIR ${CMAKE_BINARY_DIR}/bridge_scripts)

# set extra component directroies
set(EXTRA_COMPONENT_DIRS
    $ENV{IDF_PATH}/examples/system/console/advanced/components
)

# set the embedfs root
set(BRIDGE_EMBEDFS_ROOT bridge_embedfs_root)

include(${TOP_DIR}/cmake/project.cmake)
include(${TOP_DIR}/cmake/bridge.cmake)
include(${TOP_DIR}/cmake/extension.cmake)

# find luac
find_luac(LUAC)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
if(NOT $ENV{IDF_VERSION} STREQUAL ${REQUIRED_IDF_VERSION})
message(FATAL_ERROR "IDF version must be v${REQUIRED_IDF_VERSION}, current version is v$ENV{IDF_VERSION}")
endif()

add_definitions(-DBRIDGE_EMBEDFS_ROOT=${BRIDGE_EMBEDFS_ROOT})
add_definitions(-DBRIDGE_VERSION="${PROJECT_VER}")
if(CONFIG_HAP_MFI_HW_AUTH)
    add_definitions(-DHAVE_MFI_HW_AUTH=1)
endif()
project(${PROJECT})

set(SCRIPTS_SRC_DIRS ${BRIDGE_SCRIPTS_DIR})

target_add_lua_binary_embedfs(${PROJECT}.elf
    ${BRIDGE_EMBEDFS_ROOT}
    ${LUAC}
    DEBUG
    SRC_DIRS ${SCRIPTS_SRC_DIRS} ${PLUGINS_DIR}
)
