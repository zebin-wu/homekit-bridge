# Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

set(TOP_DIR ${COMPONENT_DIR}/../../..)
include(${TOP_DIR}/cmake/bridge.cmake)

set(APP_SRCS
    app_main.c
    app_wifi.c
    app_spiffs.c
    app_console.c
)

idf_component_register(
    SRCS ${APP_SRCS} ${BRIDGE_SRCS}
    INCLUDE_DIRS ${BRIDGE_INC_DIR}
)

add_definitions(-DHAP_LOG_LEVEL=${CONFIG_HAP_LOG_LEVEL})
add_definitions(-DIP=1)
