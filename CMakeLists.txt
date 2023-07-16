# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.8)

include(cmake/extension.cmake)

# ESP_PLATFORM is set to 1 whenever the ESP-IDF build system is being used.
if(${ESP_PLATFORM})
    set(PLATFORM esp)

    # Default target is "esp32"
    if(NOT DEFINED ${IDF_TARGET})
        set(IDF_TARGET "esp32")
    endif()

    # Set the toolchain
    set(CMAKE_TOOLCHAIN_FILE $ENV{IDF_PATH}/tools/cmake/toolchain-${IDF_TARGET}.cmake)
endif()

# project name
project("homekit-bridge"
    LANGUAGES C
    VERSION 0.1
)

set(TARGET ${CMAKE_PROJECT_NAME})

# Create a dummy file to work around CMake requirement of
# having a source file while adding an executable.
set(dummy_c_file ${CMAKE_BINARY_DIR}/dummy.c)
add_custom_command(OUTPUT ${dummy_c_file}
    COMMAND ${CMAKE_COMMAND} -E touch ${dummy_c_file}
    VERBATIM)
add_custom_target(dummy_c_file DEPENDS ${dummy_c_file})
add_executable(${TARGET} ${dummy_c_file})
add_dependencies(${TARGET} dummy_c_file)

if(NOT DEFINED PLATFORM)
    string(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM)
endif()
message("Current platform: " ${PLATFORM})

include(platform/${PLATFORM}/cmake/platform.cmake)

add_subdirectory(platform)
add_subdirectory(third_party)
add_subdirectory(bridge)

target_link_libraries(${TARGET}
    platform::common
    platform::${PLATFORM}
    third_party::HomeKitAdk
    third_party::lua
    third_party::lua-cjson
)

if(CONFIG_POSIX)
    target_link_libraries(${TARGET} platform::posix)
endif()

if(CONFIG_OPENSSL)
    target_link_libraries(${TARGET} platform::openssl)
endif()

if(CONFIG_MBEDTLS)
    target_link_libraries(${TARGET} platform::mbedtls)
endif()
