# directory
set(LUA_CJSON_DIR ${TOP_DIR}/ext/lua-cjson)
set(LUA_CJSON_SRC_DIR ${LUA_CJSON_DIR})

# collect lua-cjson sources
set(LUA_CJSON_SRCS
    ${LUA_CJSON_SRC_DIR}/lua_cjson.c
    ${LUA_CJSON_SRC_DIR}/strbuf.c
    ${LUA_CJSON_SRC_DIR}/fpconv.c
)
