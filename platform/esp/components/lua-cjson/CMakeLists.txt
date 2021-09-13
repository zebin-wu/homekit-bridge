set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/lua-cjson.cmake)

idf_component_register(
    SRCS ${LUA_CJSON_SRCS}
    PRIV_REQUIRES lua
)

# fix warning
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-implicit-fallthrough)
