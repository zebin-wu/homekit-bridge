# Copyright (c) 2021-2023 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

set(LUA_CJSON_DIR lua-cjson)

add_library(lua-cjson STATIC
    ${LUA_CJSON_DIR}/lua_cjson.c
    ${LUA_CJSON_DIR}/strbuf.c
    ${LUA_CJSON_DIR}/fpconv.c
)

target_compile_options(lua-cjson PRIVATE -Wno-implicit-fallthrough)

target_link_libraries(lua-cjson PRIVATE lua)
add_library(third_party::lua-cjson ALIAS lua-cjson)
