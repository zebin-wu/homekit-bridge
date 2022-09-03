# Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/platform.cmake)

idf_component_register(
    SRCS ${PLATFORM_ESP_SRCS}
    INCLUDE_DIRS ${PLATFORM_ESP_INC_DIRS}
    REQUIRES
    PRIV_REQUIRES app_update homekit_adk mbedtls nvs_flash console argtable3
)

add_definitions(
    -DIP=1
    -DHAP_LOG_LEVEL=${CONFIG_HAP_LOG_LEVEL}
)

# fix warning
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-implicit-fallthrough)
