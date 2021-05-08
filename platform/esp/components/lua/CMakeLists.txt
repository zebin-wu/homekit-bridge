set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/lua.cmake)

idf_component_register(
    SRCS ${LUA_SRCS}
    INCLUDE_DIRS ${LUA_INC_DIR}
)
