# Copyright (c) 2021-2022 KNpTrue and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

include(${TOP_DIR}/cmake/lua.cmake)

project(luac)

# collect sources
set(SRCS "${LUAC_SRCS}"
         "${LUA_SRCS}")
add_executable(${PROJECT_NAME} ${SRCS})

# add include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${LUA_INC_DIR}
)

# add definitions
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        LUA_USE_LINUX
)

# add link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        m
        dl
)
