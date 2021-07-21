// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <HAP.h>
#include <HAPCrypto.h>
#include <pal/memory.h>

#include "lcryptolib.h"

#define LUA_CRYPTO_AES_CTX_NAME "AESCtx"

#define LCRYPT_AES_GET_CTX(L, idx) \
    luaL_checkudata(L, idx, LUA_CRYPTO_AES_CTX_NAME)

static int lcrypt_aes_create(lua_State *L) {
    size_t keylen;
    const char *key = luaL_checklstring(L, 1, &keylen);
    size_t ivlen;
    const char *iv = luaL_checklstring(L, 2, &ivlen);
    if (ivlen != 16) {
        luaL_error(L, "the length of IV is longer than 16");
    }
    HAP_aes_ctr_ctx *ctx = lua_newuserdata(L, sizeof(HAP_aes_ctr_ctx));
    luaL_setmetatable(L, LUA_CRYPTO_AES_CTX_NAME);
    HAP_aes_ctr_init(ctx, (uint8_t *)key, (int)keylen, (uint8_t *)iv);
    return 1;
}

static int lcrypto_aes_ctx_gc(lua_State *L) {
    HAP_aes_ctr_ctx *ctx = LCRYPT_AES_GET_CTX(L, 1);
    HAP_aes_ctr_done(ctx);
    return 0;
}

static int lcrypto_aes_ctx_tostring(lua_State *L) {
    HAP_aes_ctr_ctx *ctx = LCRYPT_AES_GET_CTX(L, 1);
    lua_pushfstring(L, "AES context (%p)", ctx);
    return 1;
}

static int lcrypto_aes_ctx_encrypt(lua_State *L) {
    HAP_aes_ctr_ctx *ctx = LCRYPT_AES_GET_CTX(L, 1);
    size_t len;
    const char *in = luaL_checklstring(L, 2, &len);
    luaL_Buffer B;
    char *out = luaL_buffinitsize(L, &B, len);
    HAP_aes_ctr_encrypt(ctx, (uint8_t *)out, (const uint8_t *)in, len);
    luaL_pushresult(&B);
    return 1;
}

static int lcrypto_aes_ctx_decrypt(lua_State *L) {
    HAP_aes_ctr_ctx *ctx = LCRYPT_AES_GET_CTX(L, 1);
    size_t len;
    const char *in = luaL_checklstring(L, 2, &len);
    luaL_Buffer B;
    char *out = luaL_buffinitsize(L, &B, len);
    HAP_aes_ctr_decrypt(ctx, (uint8_t *)out, (const uint8_t *)in, len);
    luaL_pushresult(&B);
    return 1;
}

static const luaL_Reg lcrypt_aes_funcs[] = {
    {"create", lcrypt_aes_create},
    {NULL, NULL},
};

/*
 * metamethods for AES context
 */
static const luaL_Reg lcrypto_aes_ctx_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lcrypto_aes_ctx_gc},
    {"__close", lcrypto_aes_ctx_gc},
    {"__tostring", lcrypto_aes_ctx_tostring},
    {NULL, NULL}
};

/*
 * methods for AES context
 */
static const luaL_Reg lcrypto_aes_ctx_meth[] = {
    {"encrypt", lcrypto_aes_ctx_encrypt},
    {"decrypt", lcrypto_aes_ctx_decrypt},
    {NULL, NULL},
};

static void lcrypto_aes_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_CRYPTO_AES_CTX_NAME);  /* metatable for AES context */
    luaL_setfuncs(L, lcrypto_aes_ctx_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lcrypto_aes_ctx_meth);  /* create method table */
    luaL_setfuncs(L, lcrypto_aes_ctx_meth, 0);  /* add AES context methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_crypto_aes(lua_State *L) {
    luaL_newlib(L, lcrypt_aes_funcs); /* new module */
    lcrypto_aes_createmeta(L);
    return 1;
}
