# Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

include(cmake/project.cmake)

project(${PROJECT})

string(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM)

add_subdirectory(platform/${PLATFORM})
