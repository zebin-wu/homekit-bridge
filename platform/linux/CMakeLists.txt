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
    src/nvs.c
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
