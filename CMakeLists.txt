# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

include(cmake/project.cmake)

project(${PROJECT_NAME})

string(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM)

add_subdirectory(platform/${PLATFORM})
