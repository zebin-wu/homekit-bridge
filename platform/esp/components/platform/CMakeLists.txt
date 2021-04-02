set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/platform.cmake)

set(PLATFORM_SRCS "src/board.c")

idf_component_register(SRCS "${PLATFORM_SRCS}"
                INCLUDE_DIRS "${PLATFORM_INC_DIR}"
                REQUIRES
                PRIV_REQUIRES app_update)
