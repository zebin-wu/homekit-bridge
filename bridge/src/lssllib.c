// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <pal/crypto/ssl.h>
#include <lauxlib.h>
#include <HAPLog.h>
#include <HAPBase.h>
#include "app_int.h"
#include "lc.h"

#define LUA_SSL_CTX_NAME "SSL*"
#define LSSL_CTX_COMMON(L, ctx, in, ilen, method) \
    lssl_ctx_common(L, ctx, in, ilen, #method, pal_ssl_##method)

typedef struct {
    pal_ssl_ctx *ctx;
} lssl_ctx;

typedef pal_ssl_err (*lssl_func)(pal_ssl_ctx *ctx, const void *in, size_t ilen, void *out, size_t *olen);

const char *lssl_endpoint_strs[] = {
    "client",
    "server",
    NULL,
};

static int lssl_create(lua_State *L) {
    pal_ssl_endpoint ep = luaL_checkoption(L, 1, NULL, lssl_endpoint_strs);
    const char *hostname = NULL;
    if (!lua_isnoneornil(L, 2)) {
        hostname = luaL_checkstring(L, 2);
    }

    lssl_ctx *ctx = lua_newuserdata(L, sizeof(*ctx));
    luaL_setmetatable(L, LUA_SSL_CTX_NAME);
    ctx->ctx = pal_ssl_create(ep, hostname);
    if (!ctx) {
        luaL_error(L, "failed to create SSL context");
    }
    return 1;
}

static int lssl_ctx_finshed(lua_State *L) {
    lssl_ctx *ctx = luaL_checkudata(L, 1, LUA_SSL_CTX_NAME);
    lua_pushboolean(L, pal_ssl_finshed(ctx->ctx));
    return 1;
}

static int lssl_ctx_common(lua_State *L, lssl_ctx *ctx,
    const void *in, size_t ilen, const char *method, lssl_func func) {
    char out[512];
    luaL_Buffer B;
    luaL_buffinit(L, &B);
    while (1) {
        size_t olen = sizeof(out);
        pal_ssl_err err = func(ctx->ctx, in, ilen, out, &olen);
        switch (err) {
        case PAL_SSL_ERR_OK:
            if (olen == 0) {
                return 0;
            }
            luaL_addlstring(&B, out, olen);
            luaL_pushresult(&B);
            return 1;
        case PAL_SSL_ERR_AGAIN:
            in = NULL;
            ilen = 0;
            luaL_addlstring(&B, out, olen);
            break;
        default:
            luaL_error(L, "failed to %s", method);
        }
    }
    return 1;
}

static int lssl_ctx_handshake(lua_State *L) {
    lssl_ctx *ctx = luaL_checkudata(L, 1, LUA_SSL_CTX_NAME);
    size_t ilen = 0;
    const char *in = NULL;
    if (!lua_isnoneornil(L, 2)) {
        in = luaL_checklstring(L, 2, &ilen);
    }
    if (pal_ssl_finshed(ctx->ctx)) {
        luaL_error(L, "handshake is finshed");
    }
    return LSSL_CTX_COMMON(L, ctx, in, ilen, handshake);
}

static int lssl_ctx_encrypt(lua_State *L) {
    lssl_ctx *ctx = luaL_checkudata(L, 1, LUA_SSL_CTX_NAME);
    size_t ilen = 0;
    const char *in = luaL_checklstring(L, 2, &ilen);
    return LSSL_CTX_COMMON(L, ctx, in, ilen, encrypt);
}

static int lssl_ctx_decrypt(lua_State *L) {
    lssl_ctx *ctx = luaL_checkudata(L, 1, LUA_SSL_CTX_NAME);
    size_t ilen;
    const char *in = luaL_checklstring(L, 2, &ilen);
    return LSSL_CTX_COMMON(L, ctx, in, ilen, decrypt);
}

static int lssl_ctx_gc(lua_State *L) {
    lssl_ctx *ctx = luaL_checkudata(L, 1, LUA_SSL_CTX_NAME);
    if (ctx->ctx) {
        pal_ssl_free(ctx->ctx);
        ctx->ctx = NULL;
    }
    return 0;
}

static int lssl_ctx_tostring(lua_State *L) {
    lssl_ctx *ctx = luaL_checkudata(L, 1, LUA_SSL_CTX_NAME);
    if (ctx->ctx) {
        lua_pushfstring(L, "SSL (%p)", ctx->ctx);
    } else {
        lua_pushliteral(L, "SSL (destroyed)");
    }
    return 1;
}

static const luaL_Reg lssl_funcs[] = {
    {"create", lssl_create},
    {NULL, NULL},
};

/*
 * metamethods for message queue context
 */
static const luaL_Reg lssl_ctx_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lssl_ctx_gc},
    {"__tostring", lssl_ctx_tostring},
    {NULL, NULL}
};

/*
 * methods for SSL context
 */
static const luaL_Reg lssl_ctx_meth[] = {
    {"finshed", lssl_ctx_finshed},
    {"handshake", lssl_ctx_handshake},
    {"encrypt", lssl_ctx_encrypt},
    {"decrypt", lssl_ctx_decrypt},
    {NULL, NULL},
};

static void lssl_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_SSL_CTX_NAME);  /* metatable for SSL context */
    luaL_setfuncs(L, lssl_ctx_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lssl_ctx_meth);  /* create method table */
    luaL_setfuncs(L, lssl_ctx_meth, 0);  /* add SSL context methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_ssl(lua_State *L) {
    luaL_newlib(L, lssl_funcs);
    lssl_createmeta(L);
    return 1;
}
