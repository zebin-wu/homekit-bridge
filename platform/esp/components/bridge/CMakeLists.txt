set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/bridge.cmake)

idf_component_register(SRCS "${BRIDGE_SRCS}"
                INCLUDE_DIRS "${BRIDGE_INC_DIR}"
                REQUIRES
                PRIV_REQUIRES homekit_adk platform)

add_definitions(-DHAP_LOG_LEVEL=${CONFIG_HAP_LOG_LEVEL})
