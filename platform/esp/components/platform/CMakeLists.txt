set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/platform.cmake)

idf_component_register(
    SRCS ${PLATFORM_ESP_SRCS}
    INCLUDE_DIRS ${PLATFORM_INC_DIR}
    REQUIRES
    PRIV_REQUIRES app_update homekit_adk mbedtls
)

add_definitions(
    -DIP=1
    -DHAP_LOG_LEVEL=${CONFIG_HAP_LOG_LEVEL}
)
