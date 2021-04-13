set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/datastruct.cmake)

idf_component_register(SRCS "${DATASTRUCT_SRCS}"
                INCLUDE_DIRS "${DATASTRUCT_INC_DIR}"
                REQUIRES
                PRIV_REQUIRES)
