// Copyright (c) 2021-2022 KNpTrue and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <pal/cipher.h>

#define LCIPHER_CTX_NAME "CipherContext*"

#define LCIPHER_GET_CTX(L, idx) \
    luaL_checkudata(L, idx, LCIPHER_CTX_NAME)

static const char *lcipher_type_strs[] = {
    "AES-128-ECB",
    "AES-192-ECB",
    "AES-256-ECB",
    "AES-128-CBC",
    "AES-192-CBC",
    "AES-256-CBC",
    "AES-128-CFB128",
    "AES-192-CFB128",
    "AES-256-CFB128",
    "AES-128-CTR",
    "AES-192-CTR",
    "AES-256-CTR",
    "AES-128-GCM",
    "AES-192-GCM",
    "AES-256-GCM",
    "CAMELLIA-128-ECB",
    "CAMELLIA-192-ECB",
    "CAMELLIA-256-ECB",
    "CAMELLIA-128-CBC",
    "CAMELLIA-192-CBC",
    "CAMELLIA-256-CBC",
    "CAMELLIA-128-CFB128",
    "CAMELLIA-192-CFB128",
    "CAMELLIA-256-CFB128",
    "CAMELLIA-128-CTR",
    "CAMELLIA-192-CTR",
    "CAMELLIA-256-CTR",
    "CAMELLIA-128-GCM",
    "CAMELLIA-192-GCM",
    "CAMELLIA-256-GCM",
    "DES-ECB",
    "DES-CBC",
    "DES-EDE-ECB",
    "DES-EDE-CBC",
    "DES-EDE3-ECB",
    "DES-EDE3-CBC",
    "BLOWFISH-ECB",
    "BLOWFISH-CBC",
    "BLOWFISH-CFB64",
    "BLOWFISH-CTR",
    "ARC4-128",
    "AES-128-CCM",
    "AES-192-CCM",
    "AES-256-CCM",
    "CAMELLIA-128-CCM",
    "CAMELLIA-192-CCM",
    "CAMELLIA-256-CCM",
    "ARIA-128-ECB",
    "ARIA-192-ECB",
    "ARIA-256-ECB",
    "ARIA-128-CBC",
    "ARIA-192-CBC",
    "ARIA-256-CBC",
    "ARIA-128-CFB128",
    "ARIA-192-CFB128",
    "ARIA-256-CFB128",
    "ARIA-128-CTR",
    "ARIA-192-CTR",
    "ARIA-256-CTR",
    "ARIA-128-GCM",
    "ARIA-192-GCM",
    "ARIA-256-GCM",
    "ARIA-128-CCM",
    "ARIA-192-CCM",
    "ARIA-256-CCM",
    "AES-128-OFB",
    "AES-192-OFB",
    "AES-256-OFB",
    "AES-128-XTS",
    "AES-256-XTS",
    "CHACHA20",
    "CHACHA20-POLY1305",
    NULL,
};

static const char *lcipher_padding_strs[] = {
    "NONE",
    "PKCS7",
    "ISO7816_4",
    "ANSI923",
    "ZERO",
    NULL
};

static const char *lcipher_op_strs[] = {
    "encrypt",
    "decrypt",
    NULL,
};

typedef struct {
    pal_cipher_ctx *ctx;
} lcipher_ctx;

static int lcipher_create(lua_State *L) {
    pal_cipher_type type = luaL_checkoption(L, 1, NULL, lcipher_type_strs);
    lcipher_ctx *ctx = lua_newuserdata(L, sizeof(lcipher_ctx));
    luaL_setmetatable(L, LCIPHER_CTX_NAME);

    ctx->ctx = pal_cipher_new(type);
    if (!ctx->ctx)  {
        luaL_error(L, "Failed to create a %s cipher.", lcipher_type_strs[type]);
    }
    return 1;
}

static int lcipher_ctx_gc(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    pal_cipher_free(ctx->ctx);
    return 0;
}

static int lcipher_ctx_tostring(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    lua_pushfstring(L, "cipher context (%p)", ctx->ctx);
    return 1;
}

static int lcipher_ctx_get_key_len(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    lua_pushinteger(L, pal_cipher_get_key_len(ctx->ctx));
    return 1;
}

static int lcipher_ctx_get_iv_len(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    lua_pushinteger(L, pal_cipher_get_iv_len(ctx->ctx));
    return 1;
}

static int lcipher_ctx_set_padding(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    pal_cipher_padding padding = luaL_checkoption(L, 2, "NONE", lcipher_padding_strs);
    if (!pal_cipher_set_padding(ctx->ctx, padding)) {
        luaL_error(L, "Failed to set padding to the cipher.");
    }
    return 1;
}

static int lcipher_ctx_begin(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    pal_cipher_operation op = luaL_checkoption(L, 2, NULL, lcipher_op_strs);
    size_t keylen;
    const char *key = luaL_checklstring(L, 3, &keylen);
    if (pal_cipher_get_key_len(ctx->ctx) != keylen) {
        luaL_error(L, "invalid key length");
    }
    const char *iv = NULL;
    if (pal_cipher_get_iv_len(ctx->ctx) == 0) {
        goto begin;
    }
    size_t ivlen;
    iv = luaL_checklstring(L, 4, &ivlen);
    if (pal_cipher_get_iv_len(ctx->ctx) != ivlen) {
        luaL_error(L, "invalid IV length");
    }

begin:
    if (!pal_cipher_begin(ctx->ctx, op,
        (const uint8_t *)key, (const uint8_t *)iv)) {
        luaL_error(L, "Failed to begin a %s process.", lcipher_op_strs[op]);
    }
    return 1;
}

static int lcipher_ctx_update(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    size_t inlen;
    const char *in = luaL_checklstring(L, 2, &inlen);
    size_t outlen = inlen + pal_cipher_get_block_size(ctx->ctx);
    char out[outlen];
    if (!pal_cipher_update(ctx->ctx, in, inlen, out, &outlen)) {
        luaL_error(L, "Failed to update data to the cipher.");
    }
    lua_pushlstring(L, out, outlen);
    return 1;
}

static int lcipher_ctx_finsh(lua_State *L) {
    lcipher_ctx *ctx = LCIPHER_GET_CTX(L, 1);
    size_t outlen = pal_cipher_get_block_size(ctx->ctx);
    char out[outlen];
    if (!pal_cipher_finsh(ctx->ctx, out, &outlen)) {
        luaL_error(L, "Failed to finsh the process.");
    }
    lua_pushlstring(L, out, outlen);
    return 1;
}

/*
 * metamethods for cipher context
 */
static const luaL_Reg lcipher_ctx_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lcipher_ctx_gc},
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
