// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <util_base64.h>

static int lbase64_encode(lua_State *L) {
    size_t ilen;
    const char *input = luaL_checklstring(L, 1, &ilen);
    luaL_Buffer B;
    luaL_buffinitsize(L, &B, util_base64_encoded_len(ilen));
    util_base64_encode(input, ilen, luaL_buffaddr(&B), B.size, &luaL_bufflen(&B));
    luaL_pushresult(&B);
    return 1;
}

static int lbase64_decode(lua_State *L) {
    size_t ilen;
    const char *input = luaL_checklstring(L, 1, &ilen);
    luaL_Buffer B;
    luaL_buffinitsize(L, &B, ilen);
    if (luai_unlikely(util_base64_decode(input, ilen,
        luaL_buffaddr(&B), B.size, &luaL_bufflen(&B)) != kHAPError_None)) {
        luaL_error(L, "failed to decode");
    }
    luaL_pushresult(&B);
    return 1;
}

static const luaL_Reg lbase64_funcs[] = {
    {"encode", lbase64_encode},
    {"decode", lbase64_decode},
    {NULL, NULL},
};

LUAMOD_API int luaopen_base64(lua_State *L) {
    luaL_newlib(L, lbase64_funcs);
    return 1;
}
