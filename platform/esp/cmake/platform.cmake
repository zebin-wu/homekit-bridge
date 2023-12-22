# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

# system api
set(CONFIG_POSIX ON)

# crypto library
set(CONFIG_OPENSSL OFF)
set(CONFIG_MBEDTLS ON)

# export compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set the embedfs root
set(BRIDGE_EMBEDFS_ROOT bridge_embedfs_root)

include($ENV{IDF_PATH}/tools/cmake/idf.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/extension.cmake)

# Enable the component manager for regular projects if not explicitly disabled.
if(NOT "$ENV{IDF_COMPONENT_MANAGER}" EQUAL "0")
    idf_build_set_property(IDF_COMPONENT_MANAGER 1)
endif()

# Set component manager interface version
idf_build_set_property(__COMPONENT_MANAGER_INTERFACE_VERSION 2)

# Set environment
set(ENV{IDF_TARGET} ${IDF_TARGET})
set(ENV{PYTHON_DEPS_CHECKED} ${PYTHON_DEPS_CHECKED})
set(ENV{CCACHE_ENABLE} ${CCACHE_ENABLE})

idf_build_component(${CMAKE_CURRENT_LIST_DIR}/../dependencies)
set(components dependencies)

idf_build_process(${IDF_TARGET}
    COMPONENTS idf::spiffs
    SDKCONFIG ${CMAKE_SOURCE_DIR}/sdkconfig
    SDKCONFIG_DEFAULTS ${CMAKE_CURRENT_LIST_DIR}/../sdkconfig.defaults
    BUILD_DIR ${CMAKE_BINARY_DIR}
)

target_link_libraries(${TARGET} idf::freertos idf::spi_flash idf::esp_phy)

# Attach additional targets to the executable file for flashing,
# linker script generation, partition_table generation, etc.
idf_build_executable(${TARGET})

# Generate project_description.json
project_info("${test_components}")

# Add idf commands
add_idf_commands()

# Init compile options
init_compiler_options()

# generate storage partition
set(STORAGE_DIR ${CMAKE_BINARY_DIR}/storage)
make_directory(${STORAGE_DIR})
spiffs_create_partition_image(storage
    ${STORAGE_DIR}
    FLASH_IN_PROJECT
)
