set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/platform.cmake)

set(SRCS
    src/board.c
    src/memory.c
)

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${PLATFORM_INC_DIR}
    REQUIRES
    PRIV_REQUIRES app_update
)
