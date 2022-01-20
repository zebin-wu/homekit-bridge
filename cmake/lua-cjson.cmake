# Copyright (c) 2021-2022 KNpTrue and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

# directory
set(LUA_CJSON_DIR ${TOP_DIR}/ext/lua-cjson)
set(LUA_CJSON_SRC_DIR ${LUA_CJSON_DIR})

# collect lua-cjson sources
set(LUA_CJSON_SRCS
    ${LUA_CJSON_SRC_DIR}/lua_cjson.c
    ${LUA_CJSON_SRC_DIR}/strbuf.c
    ${LUA_CJSON_SRC_DIR}/fpconv.c
)
