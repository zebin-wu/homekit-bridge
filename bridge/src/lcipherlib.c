// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <pal/cipher.h>

#include "lcipherlib.h"

#define LCIPHER_CTX_NAME "CipherCtx"

#define LCIPHER_GET_CTX(L, idx) \
    luaL_checkudata(L, idx, LCIPHER_CTX_NAME)

static const char *lcipher_type_strs[] = {
    "NONE",
    "AES-128-CBC",
};

static const char *lcipher_padding_strs[] = {
    "NONE",
    "PKCS7",
    "ISO7816_4",
    "ANSI923",
    "ZERO",
};

static const char *lcipher_op_strs[] = {
    "encrypt",
    "decrypt",
};

typedef struct {
    pal_cipher_ctx *ctx;
    bool in_progress;
} lcipher_ctx;

static pal_cipher_type lcipher_lookup_type(const char *s) {
    for (size_t i = 0; i < HAPArrayCount(lcipher_type_strs); i++) {
        if (HAPStringAreEqual(s, lcipher_type_strs[i])) {
            return i;
        }
    }
    return PAL_CIPHER_TYPE_MAX;
}

static pal_cipher_padding lcipher_lookup_padding(const char *s) {
    for (size_t i = 0; i < HAPArrayCount(lcipher_padding_strs); i++) {
        if (HAPStringAreEqual(s, lcipher_padding_strs[i])) {
            return i;
        }
    }
    return PAL_CIPHER_PADDING_MAX;
}

static pal_cipher_operation lcipher_lookup_op(const char *s) {
    for (size_t i = 0; i < HAPArrayCount(lcipher_op_strs); i++) {
        if (HAPStringAreEqual(s, lcipher_op_strs[i])) {
            return i;
        }
    }
    return PAL_CIPHER_OP_NONE;
}

static int lcipher_create(lua_State *L) {
    const char *s = luaL_checkstring(L, 1);
    pal_cipher_type type = lcipher_lookup_type(s);
    if (type == PAL_CIPHER_TYPE_MAX) {
        luaL_error(L, "invalid type");
    }
    lcipher_ctx *ctx = lua_newuserdata(L, sizeof(lcipher_ctx));
    luaL_setmetatable(L, LCIPHER_CTX_NAME);

    ctx->ctx = pal_cipher_new(type);
    if (!ctx->ctx)  {
        goto err;
    }
    ctx->in_progress = false;
    return 1;

err:
    luaL_pushfail(L);
    return 1;
}

static int lcipher_ctx_gc(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    pal_cipher_free(ctx->ctx);
    ctx->ctx = NULL;
    ctx->in_progress = false;
    return 0;
}

static int lcipher_ctx_tostring(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    if (ctx->ctx) {
        lua_pushfstring(L, "cipher context (%p)", ctx->ctx);
    } else {
        lua_pushliteral(L, "cipher context (destoryed)");
    }
    return 1;
}

static int lcipher_ctx_get_key_len(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    if (!ctx->ctx) {
        luaL_error(L, "attempt to use a destoryed cipher context");
    }
    lua_pushinteger(L, pal_cipher_get_key_len(ctx->ctx));
    return 1;
}

static int lcipher_ctx_get_iv_len(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    if (!ctx->ctx) {
        luaL_error(L, "attempt to use a destoryed cipher context");
    }
    lua_pushinteger(L, pal_cipher_get_iv_len(ctx->ctx));
    return 1;
}

static int lcipher_ctx_set_padding(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    if (!ctx->ctx) {
        luaL_error(L, "attempt to use a destoryed cipher context");
    }
    const char *s = luaL_checkstring(L, 2);
    pal_cipher_padding padding = lcipher_lookup_padding(s);
    if (padding == PAL_CIPHER_PADDING_MAX) {
        luaL_error(L, "invalid padding");
    }
    lua_pushboolean(L, pal_cipher_set_padding(ctx->ctx, padding));
    return 1;
}

static int lcipher_ctx_begin(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    if (!ctx->ctx) {
        luaL_error(L, "attempt to use a destoryed cipher context");
    }
    const char *s = luaL_checkstring(L, 2);
    pal_cipher_operation op = lcipher_lookup_op(s);
    if (op == PAL_CIPHER_OP_NONE) {
        luaL_error(L, "invalid operation");
    }
    const char *key = luaL_checkstring(L, 3);
    const char *iv = luaL_checkstring(L, 4);
    lua_pushboolean(L, pal_cipher_begin(ctx->ctx, op, (const uint8_t *)key, (const uint8_t *)iv));
    return 1;
}

static int lcipher_ctx_update(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    if (!ctx->ctx) {
        luaL_error(L, "attempt to use a destoryed cipher context");
    }
    size_t inlen;
    const char *in = luaL_checklstring(L, 2, &inlen);
    size_t outlen = inlen + pal_cipher_get_block_size(ctx->ctx);
    char out[outlen];
    if (!pal_cipher_update(ctx->ctx, in, inlen, out, &outlen)) {
        goto err;
    }
    lua_pushlstring(L, out, outlen);
    return 1;

err:
    luaL_pushfail(L);
    return 1;
}

static int lcipher_ctx_finsh(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    if (!ctx->ctx) {
        luaL_error(L, "attempt to use a destoryed cipher context");
    }
    size_t outlen = pal_cipher_get_block_size(ctx->ctx);
    char out[outlen];
    if (!pal_cipher_finsh(ctx->ctx, out, &outlen)) {
        goto err;
    }
    lua_pushlstring(L, out, outlen);
    return 1;

err:
    luaL_pushfail(L);
    return 1;
}

/*
 * metamethods for cipher context
 */
static const luaL_Reg lcipher_ctx_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lcipher_ctx_gc},
    {"__close", lcipher_ctx_gc},
    {"__tostring", lcipher_ctx_tostring},
    {NULL, NULL}
};

/*
 * methods for cipher context
 */
static const luaL_Reg lcipher_ctx_meth[] = {
    {"getKeyLen", lcipher_ctx_get_key_len},
    {"getIVLen", lcipher_ctx_get_iv_len},
    {"setPadding", lcipher_ctx_set_padding},
    {"begin", lcipher_ctx_begin},
    {"update", lcipher_ctx_update},
    {"finsh", lcipher_ctx_finsh},
    {NULL, NULL},
};

static const luaL_Reg lcipher_funcs[] = {
    {"create", lcipher_create},
    {NULL, NULL}
};

static void lcipher_createmeta(lua_State *L) {
    luaL_newmetatable(L, LCIPHER_CTX_NAME);  /* metatable for cipher context */
    luaL_setfuncs(L, lcipher_ctx_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lcipher_ctx_meth);  /* create method table */
    luaL_setfuncs(L, lcipher_ctx_meth, 0);  /* add cipher context methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_cipher(lua_State *L) {
    luaL_newlib(L, lcipher_funcs); /* new module */
    lcipher_createmeta(L);
    return 1;
}
