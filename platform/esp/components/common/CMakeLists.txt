set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/common.cmake)

idf_component_register(SRCS "${COMMON_SRCS}"
                INCLUDE_DIRS "${COMMON_INC_DIR}"
                REQUIRES
                PRIV_REQUIRES lua)
