# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

add_library(platform_linux STATIC
    src/chip.c
    src/dns.c
    src/hap.c
    src/main.c
    src/net_if.c
)

target_include_directories(platform_linux PUBLIC include)
target_link_libraries(platform_linux PRIVATE bridge platform third_party::HomeKitAdk)

if(CONFIG_POSIX)
    target_link_libraries(platform_linux PRIVATE platform::posix)
endif()

if(CONFIG_OPENSSL)
    target_link_libraries(platform_linux PRIVATE platform::openssl ssl crypto)
endif()

if(CONFIG_MBEDTLS)
    target_sources(platform_linux PRIVATE src/mbedtls/ssl.c)
endif()

target_compile_definitions(platform_linux PRIVATE
    BRIDGE_WORK_DIR="${BRIDGE_WORK_DIR}"
)

add_library(platform::linux ALIAS platform_linux)

# # Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
# #
# # Licensed under the Apache License, Version 2.0 (the “License”);
# # you may not use this file except in compliance with the License.
# # See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

# # Set the minimum version of CMake that can be used
# # To find the cmake version run
# # $ cmake --version
# cmake_minimum_required(VERSION 3.5)

# set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)
# set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
# set(PLUGINS_DIR ${TOP_DIR}/plugins)

# include(${TOP_DIR}/cmake/project.cmake)
# include(${TOP_DIR}/cmake/adk.cmake)
# include(${TOP_DIR}/cmake/bridge.cmake)
# include(${TOP_DIR}/cmake/lua.cmake)
# include(${TOP_DIR}/cmake/lua-cjson.cmake)
# include(${TOP_DIR}/cmake/argtable3.cmake)
# include(${TOP_DIR}/cmake/platform.cmake)
# include(${TOP_DIR}/cmake/extension.cmake)

# project(${PROJECT}-${PLATFORM})

# add_executable(${PROJECT} "")

# # set the work directory
# set(BRIDGE_WORK_DIR "/usr/local/lib/${PROJECT}")

# # set the generated scripts directory
# set(SCRIPTS_DIR ${CMAKE_BINARY_DIR}/bridge_scripts)
# set(EXAMPLE_SCRIPTS_DIR ${CMAKE_BINARY_DIR}/example_scripts)
# set(TESTS_SCRIPTS_DIR ${CMAKE_BINARY_DIR}/tests_scripts)

# # set the embedfs root
# set(BRIDGE_EMBEDFS_ROOT bridge_embedfs_root)

# # find luac
# find_luac(LUAC)

# target_add_lua_binary_embedfs(${PROJECT}
#     ${BRIDGE_EMBEDFS_ROOT}
#     ${LUAC}
#     DEBUG
#     SRC_DIRS ${BRIDGE_SCRIPTS_DIR} ${PLUGINS_DIR}
# )

# gen_lua_binary_from_dir(example_scripts
#     ${EXAMPLE_SCRIPTS_DIR}
#     ${LUAC}
#     DEBUG
#     SRC_DIRS ${TOP_DIR}/example
# )

# gen_lua_binary_from_dir(tests_scripts
#     ${TESTS_SCRIPTS_DIR}
#     ${LUAC}
#     DEBUG
#     SRC_DIRS ${TOP_DIR}/tests
# )

# # collect sources
# target_sources(${PROJECT}
#     PRIVATE
#         ${PLATFORM_LINUX_SRCS}
#         ${ADK_SRCS}
#         ${ADK_PAL_LINUX}
#         ${ADK_PAL_OPENSSL_SRCS}
#         ${BRIDGE_SRCS}
#         ${LUA_SRCS}
#         ${LUA_CJSON_SRCS}
#         ${ARGTABLE3_SRCS}
# )

# # check code style
# target_check_cstyle(${PROJECT}
#     FILES
#         ${BRIDGE_SRCS}
#         ${BRIDGE_HEADERS}
#         ${PLATFORM_LINUX_SRCS}
#         ${PLATFORM_HEADERS}
# )

# # add include directories
# target_include_directories(${PROJECT}
#     PRIVATE
#         ${ADK_INC_DIRS}
#         ${ADK_PAL_LINUX_DIR}
#         ${BRIDGE_INC_DIR}
#         ${LUA_INC_DIR}
#         ${ARGTABLE3_INC_DIR}
#         ${PLATFORM_LINUX_INC_DIRS}
# )

# # add definitions
# target_compile_definitions(${PROJECT}
#     PRIVATE
#         BRIDGE_VERSION="${PROJECT_VER}"
#         BRIDGE_WORK_DIR="${BRIDGE_WORK_DIR}"
#         BRIDGE_EMBEDFS_ROOT=${BRIDGE_EMBEDFS_ROOT}
#         IP=1
#         HAP_LOG_LEVEL=3
#         LUA_USE_LINUX
# )

# # add link libraries
# target_link_libraries(${PROJECT}
#     PRIVATE
#         pthread
#         ssl
#         crypto
#         m
#         dns_sd
#         dl
# )

# # add compile options
# target_compile_options(${PROJECT}
#     PRIVATE
#         -Wall
#         -Werror
#         -Wno-deprecated-declarations
# )

# # install binaries
# install(TARGETS ${PROJECT}
#     DESTINATION bin
# )
