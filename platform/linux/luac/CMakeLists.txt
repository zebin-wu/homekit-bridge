# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

include(${TOP_DIR}/cmake/lua.cmake)

set(PROJECT luac)

project(${PROJECT})

include_directories(${LUA_INC_DIR})

add_definitions(-DLUA_USE_LINUX)

#collect sources
set(SRCS "${LUAC_SRCS}"
         "${LUA_SRCS}")
add_executable(${PROJECT} ${SRCS})

target_link_libraries(${PROJECT} PRIVATE m dl)
