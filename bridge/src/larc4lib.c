// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <lauxlib.h>

#define LARC4_CTX_NAME "ARC4Context*"

typedef struct {
    int x;
    int y;
    size_t ndrop;
    uint8_t m[256];
} larc4_context;

static void larc4_raw_setup(larc4_context *ctx, const uint8_t *key, size_t keylen) {
    int i, j, a;
    size_t k;
    uint8_t *m;

    ctx->x = 0;
    ctx->y = 0;
    m = ctx->m;

    for (i = 0; i < 256; i++) {
        m[i] = (uint8_t)i;
    }

    j = k = 0;

    for (i = 0; i < 256; i++, k++) {
        if (k >= keylen) {
            k = 0;
        }

        a = m[i];
        j = (j + a + key[k]) & 0xFF;
        m[i] = m[j];
        m[j] = (uint8_t)a;
    }
}

static void  larc4_raw_drop(larc4_context *ctx) {
    int x, y, a, b;
    size_t i;
    size_t len;
    uint8_t *m;

    x = ctx->x;
    y = ctx->y;
    m = ctx->m;
    len = ctx->ndrop;

    for (i = 0; i < len; i++) {
        x = (x + 1) & 0xFF; a = m[x];
        y = (y + a) & 0xFF; b = m[y];

        m[x] = (uint8_t)b;
        m[y] = (uint8_t)a;
    }

    ctx->x = x;
    ctx->y = y;
}

static void larc4_raw_crypt(larc4_context *ctx, size_t len, const uint8_t *input, uint8_t *output) {
    int x, y, a, b;
    size_t i;
    uint8_t *m;

    x = ctx->x;
    y = ctx->y;
    m = ctx->m;

    for (i = 0; i < len; i++) {
        x = (x + 1) & 0xFF; a = m[x];
        y = (y + a) & 0xFF; b = m[y];

        m[x] = (uint8_t)b;
        m[y] = (uint8_t)a;

        output[i] = (uint8_t)(input[i] ^ m[(uint8_t)(a + b)]);
    }

    ctx->x = x;
    ctx->y = y;
}

static int larc4_create(lua_State *L) {
    size_t keylen;
    const char *key = luaL_checklstring(L, 1, &keylen);
    lua_Integer ndrop = luaL_optinteger(L, 2, 0);
    luaL_argcheck(L, ndrop >= 0 && ndrop <= UINT32_MAX, 2, "ndrop out of range");

    larc4_context *ctx = lua_newuserdata(L, sizeof(*ctx));
    luaL_setmetatable(L, LARC4_CTX_NAME);
    lua_pushvalue(L, 1);
    lua_setuservalue(L, -2);

    ctx->ndrop = ndrop;
    larc4_raw_setup(ctx, (const uint8_t *)key, keylen);
    if (ndrop > 0) {
        larc4_raw_drop(ctx);
    }
    return 1;
}

static int larc4_ctx_crypt(lua_State *L) {
    larc4_context *ctx = luaL_checkudata(L, 1, LARC4_CTX_NAME);
    size_t ilen;
    const char *in = luaL_checklstring(L, 2, &ilen);

    luaL_Buffer B;
    luaL_buffinitsize(L, &B, ilen);
    larc4_raw_crypt(ctx, ilen, (const uint8_t *)in, (uint8_t *)luaL_buffaddr(&B));
    luaL_addsize(&B, ilen);
    luaL_pushresult(&B);
    return 1;
}

static int larc4_ctx_reset(lua_State *L) {
    larc4_context *ctx = luaL_checkudata(L, 1, LARC4_CTX_NAME);
    lua_getuservalue(L, 1);
    size_t keylen;
    const char *key = lua_tolstring(L, -1, &keylen);
    larc4_raw_setup(ctx, (const uint8_t *)key, keylen);
    if (ctx->ndrop > 0) {
        larc4_raw_drop(ctx);
    }
    return 0;
}

static int larc4_ctx_tostring(lua_State *L) {
    larc4_context *ctx = luaL_checkudata(L, 1, LARC4_CTX_NAME);
    lua_pushfstring(L, "arc4 context (%p)", ctx);
    return 1;
}

static const luaL_Reg larc4_funcs[] = {
    {"create", larc4_create},
    {NULL, NULL},
};

/*
 * metamethods for ARC4 context
 */
static const luaL_Reg larc4_ctx_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__tostring", larc4_ctx_tostring},
    {NULL, NULL}
};

/*
 * methods for ARC4 context
 */
static const luaL_Reg larc4_ctx_meth[] = {
    {"crypt", larc4_ctx_crypt},
    {"reset", larc4_ctx_reset},
    {NULL, NULL},
};

static void larc4_createmeta(lua_State *L) {
    luaL_newmetatable(L, LARC4_CTX_NAME);  /* metatable for ARC4 context */
    luaL_setfuncs(L, larc4_ctx_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, larc4_ctx_meth);  /* create method table */
    luaL_setfuncs(L, larc4_ctx_meth, 0);  /* add ARC4 context methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_arc4(lua_State *L) {
    luaL_newlib(L, larc4_funcs);
    larc4_createmeta(L);
    return 1;
}
