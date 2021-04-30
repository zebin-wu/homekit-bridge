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
include(${TOP_DIR}/cmake/extension.cmake)

project(${PROJECT}-${PLATFORM})

# set the work directory
set(BRIDGE_WORK_DIR ${CMAKE_BINARY_DIR}/.bridge)

# compile luac
set(LUAC_BUILD_DIR ${CMAKE_BINARY_DIR}/luac)
set(LUAC_BINARY ${LUAC_BUILD_DIR}/luac)
compile_luac(${LUAC_BINARY} ${CMAKE_CURRENT_SOURCE_DIR}/luac ${LUAC_BUILD_DIR})

# generate lua binaries to work directory
gen_lua_binary_from_dir(${BRIDGE_WORK_DIR} ${BRIDGE_LUA_DIR} ${LUAC_BINARY})

# generate config.lua
configure_file(${TOP_DIR}/config/config.lua.in ${BRIDGE_WORK_DIR}/config.lua)

# collect sources
set(SRCS "${SRC_DIR}/main.c"
         "${SRC_DIR}/platform/board.c"
         "${ADK_SRCS}"
         "${ADK_PAL_LINUX}"
         "${ADK_PAL_OPENSSL_SRCS}"
         "${BRIDGE_SRCS}"
         "${DATASTRUCT_SRCS}"
         "${LUA_SRCS}")
add_executable(${PROJECT} ${SRCS})

# add include directories
target_include_directories(${PROJECT}
    PRIVATE
        ${ADK_INC_DIR}
        ${ADK_PAL_LINUX_DIR}
        ${BRIDGE_INC_DIR}
        ${DATASTRUCT_INC_DIR}
        ${LUA_INC_DIR}
        ${PLATFORM_INC_DIR}
)

# add definitions
target_compile_definitions(${PROJECT}
    PRIVATE
        BRIDGE_VER="${PROJECT_VER}"
        BRIDGE_WORK_DIR="${BRIDGE_WORK_DIR}"
        IP=1
        HAP_LOG_LEVEL=3
        LUA_USE_LINUX
)

# add link libraries
target_link_libraries(${PROJECT}
    PRIVATE
        pthread
        ssl
        crypto
        m
        dns_sd
        dl
)
