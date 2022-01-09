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
