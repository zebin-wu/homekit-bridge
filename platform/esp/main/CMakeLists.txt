set(TOP_DIR ${COMPONENT_DIR}/../../..)
include(${TOP_DIR}/cmake/bridge.cmake)

set(APP_SRCS
    app_main.c
    app_wifi.c
    app_spiffs.c
    app_console.c
)

set(SRCS ${APP_SRCS} ${BRIDGE_SRCS})
set(INC_DIRS ${BRIDGE_INC_DIR})

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${INC_DIRS}
)

add_definitions(-DHAP_LOG_LEVEL=${CONFIG_HAP_LOG_LEVEL})
