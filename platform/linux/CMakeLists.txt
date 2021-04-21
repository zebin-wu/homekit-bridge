# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(SRC_DIR src)

include(${TOP_DIR}/cmake/project.cmake)
include(${TOP_DIR}/cmake/adk.cmake)
include(${TOP_DIR}/cmake/bridge.cmake)
include(${TOP_DIR}/cmake/datastruct.cmake)
include(${TOP_DIR}/cmake/lua.cmake)
include(${TOP_DIR}/cmake/platform.cmake)

set(PLATFORM linux)
set(PROJECT ${PROJECT_NAME}-${PLATFORM})
set(BINARY ${PROJECT_NAME})

project(${PROJECT})

include_directories(${ADK_INC_DIR}
                    ${ADK_PAL_LINUX_DIR}
                    ${BRIDGE_INC_DIR}
                    ${DATASTRUCT_INC_DIR}
                    ${LUA_INC_DIR}
                    ${PLATFORM_INC_DIR})

#add definitions
add_definitions(-DBRIDGE_VER="${PROJECT_VER}"
                -DBRIDGE_LUA_DIR="${BRIDGE_LUA_DIR}"
                -DIP
                -DHAP_LOG_LEVEL=3)

#collect sources
set(SRCS "${SRC_DIR}/main.c"
         "${SRC_DIR}/platform/board.c"
         "${ADK_SRCS}"
         "${ADK_PAL_LINUX}"
         "${ADK_PAL_OPENSSL_SRCS}"
         "${BRIDGE_SRCS}"
         "${DATASTRUCT_SRCS}"
         "${LUA_SRCS}")
add_executable(${BINARY} ${SRCS})

target_link_libraries(${BINARY} PRIVATE pthread ssl crypto m dns_sd)
