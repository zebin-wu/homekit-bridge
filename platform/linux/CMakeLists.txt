cmake_minimum_required(VERSION 3.5)

project(homekit-bridge)

set(TOP_DIR ${CMAKE_SOURCE_DIR}/../..)
set(SRC_DIR src)

include(${TOP_DIR}/cmake/version.cmake)
include(${TOP_DIR}/cmake/adk.cmake)
include(${TOP_DIR}/cmake/bridge.cmake)
include(${TOP_DIR}/cmake/datastruct.cmake)
include(${TOP_DIR}/cmake/lua.cmake)
include(${TOP_DIR}/cmake/platform.cmake)

include_directories(${ADK_INC_DIR}
                    ${ADK_PAL_LINUX_DIR}
                    ${BRIDGE_INC_DIR}
                    ${DATASTRUCT_INC_DIR}
                    ${LUA_INC_DIR}
                    ${PLATFORM_INC_DIR})

#add definitions
add_definitions(-DBRIDGE_VER="${BRIDGE_VER}"
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
add_executable(homekit-bridge ${SRCS})

target_link_libraries(homekit-bridge PRIVATE pthread ssl crypto m dns_sd)
