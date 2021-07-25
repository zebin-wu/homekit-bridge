set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/platform.cmake)

set(SRCS
    ${PLATFORM_COMMON_SRCS}
    src/board.c
    src/memory.c
    src/crypto/aes.c
)

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${PLATFORM_INC_DIR}
    REQUIRES
    PRIV_REQUIRES app_update homekit_adk
)

add_definitions(
    -DIP=1
    -DHAP_LOG_LEVEL=${CONFIG_HAP_LOG_LEVEL}
)
