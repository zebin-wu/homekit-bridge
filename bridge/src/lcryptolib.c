// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lauxlib.h>
#include <HAPBase.h>
#include <pal/crypto/aes.h>
#include <pal/memory.h>

#include "lcryptolib.h"

#define LUA_CRYPTO_AES_CBC_CTX_NAME "AesCbcCtx"

#define LCRYPT_AES_CBC_GET_CTX(L, idx) \
    luaL_checkudata(L, idx, LUA_CRYPTO_AES_CBC_CTX_NAME)

/**
 * Crypto operation.
*/
typedef enum {
    LCRYPTO_OPT_ENCRYPT,
    LCRYPTO_OPT_DECRYPT,
    LCRYPTO_OPT_MAX,
} lcrypto_opt;

static const char *lcrypto_opt_strs[] = {
    [LCRYPTO_OPT_ENCRYPT] = "encrypt",
    [LCRYPTO_OPT_DECRYPT] = "decrypt",
};

typedef struct {
    pal_crypto_aes_ctx *ctx;
    lcrypto_opt opt;
    uint8_t iv[16];
} lcrypt_aes_cbc_ctx;

static lcrypto_opt lcrypto_lookup_opt(const char *s) {
    for (size_t i = 0; i < HAPArrayCount(lcrypto_opt_strs); i++) {
        if (HAPStringAreEqual(s, lcrypto_opt_strs[i])) {
            return i;
        }
    }
    return LCRYPTO_OPT_MAX;
}

static int lcrypt_aes_cbc(lua_State *L) {
    size_t keylen;
    size_t ivlen;
    lcrypto_opt opt = lcrypto_lookup_opt(luaL_checkstring(L, 1));
    if (opt == LCRYPTO_OPT_MAX) {
        luaL_error(L, "invalid opt");
    }
    const char *key = luaL_checklstring(L, 2, &keylen);
    const char *iv = luaL_checklstring(L, 3, &ivlen);
    if (ivlen != 16) {
        luaL_error(L, "the length of IV is longer than 16");
    }
    lcrypt_aes_cbc_ctx *ctx = lua_newuserdata(L, sizeof(lcrypt_aes_cbc_ctx));
    luaL_setmetatable(L, LUA_CRYPTO_AES_CBC_CTX_NAME);
    ctx->ctx = pal_crypto_aes_new();
    if (!ctx->ctx) {
        goto err;
    }
    ctx->opt = opt;

    switch (opt) {
    case LCRYPTO_OPT_ENCRYPT:
        pal_crypto_aes_set_enc_key(ctx->ctx, (uint8_t *)key, (int)keylen);
        break;
    case LCRYPTO_OPT_DECRYPT:
        pal_crypto_aes_set_dec_key(ctx->ctx, (uint8_t *)key, (int)keylen);
        break;
    default:
        break;
    }
    HAPRawBufferCopyBytes(ctx->iv, iv, ivlen);
    return 1;

err:
    luaL_pushfail(L);
    return 1;
}

static int lcrypto_aes_cbc_ctx_gc(lua_State *L) {
    lcrypt_aes_cbc_ctx *ctx = LCRYPT_AES_CBC_GET_CTX(L, 1);
    pal_crypto_aes_free(ctx->ctx);
    return 0;
}

static int lcrypto_aes_cbc_ctx_tostring(lua_State *L) {
    lcrypt_aes_cbc_ctx *ctx = LCRYPT_AES_CBC_GET_CTX(L, 1);
    lua_pushfstring(L, "AES CBC context (%p)", ctx);
    return 1;
}

static int lcrypto_aes_cbc_ctx_set_iv(lua_State *L) {
    lcrypt_aes_cbc_ctx *ctx = LCRYPT_AES_CBC_GET_CTX(L, 1);
    size_t ivlen;

    const char *iv = luaL_checklstring(L, 2, &ivlen);
    if (ivlen != 16) {
        luaL_error(L, "the length of IV is longer than 16");
    }
    HAPRawBufferCopyBytes(ctx->iv, iv, ivlen);
    return 0;
}

static int lcrypto_aes_cbc_ctx_get_iv(lua_State *L) {
    lcrypt_aes_cbc_ctx *ctx = LCRYPT_AES_CBC_GET_CTX(L, 1);
    lua_pushlstring(L, (char *)ctx->iv, 16);
    return 1;
}

static int lcrypto_aes_cbc_ctx_encrypt(lua_State *L) {
    lcrypt_aes_cbc_ctx *ctx = LCRYPT_AES_CBC_GET_CTX(L, 1);

    if (ctx->opt != LCRYPTO_OPT_ENCRYPT) {
        luaL_error(L, "attemp to use a non-encrypt context");
    }

    size_t len;
    const char *in = luaL_checklstring(L, 2, &len);
    if (len % 16) {
        luaL_error(L, "the input size must be a multiple of the AES block size of 16 Bytes.");
    }
    char out[len];
    pal_crypto_aes_cbc_enc(ctx->ctx, ctx->iv, (const uint8_t *)in, (uint8_t *)out, len);
    lua_pushlstring(L, out, len);
    return 1;
}

static int lcrypto_aes_cbc_ctx_decrypt(lua_State *L) {
    lcrypt_aes_cbc_ctx *ctx = LCRYPT_AES_CBC_GET_CTX(L, 1);

    if (ctx->opt != LCRYPTO_OPT_DECRYPT) {
        luaL_error(L, "attemp to use a non-decrypt context");
    }

    size_t len;
    const char *in = luaL_checklstring(L, 2, &len);
    if (len % 16) {
        luaL_error(L, "the input size must be a multiple of the AES block size of 16 Bytes.");
    }
    char out[len];
    pal_crypto_aes_cbc_dec(ctx->ctx, ctx->iv, (const uint8_t *)in, (uint8_t *)out, len);
    lua_pushlstring(L, out, len);
    return 1;
}

static const luaL_Reg lcrypt_aes_funcs[] = {
    {"cbc", lcrypt_aes_cbc},
    {NULL, NULL},
};

/*
 * metamethods for AES CBC context
 */
static const luaL_Reg lcrypto_aes_cbc_ctx_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lcrypto_aes_cbc_ctx_gc},
    {"__close", lcrypto_aes_cbc_ctx_gc},
    {"__tostring", lcrypto_aes_cbc_ctx_tostring},
    {NULL, NULL}
};

/*
 * methods for AES CBC context
 */
static const luaL_Reg lcrypto_aes_cbc_ctx_meth[] = {
    {"setIV", lcrypto_aes_cbc_ctx_set_iv},
    {"getIV", lcrypto_aes_cbc_ctx_get_iv},
    {"encrypt", lcrypto_aes_cbc_ctx_encrypt},
    {"decrypt", lcrypto_aes_cbc_ctx_decrypt},
    {NULL, NULL},
};

static void lcrypto_aes_createmeta(lua_State *L) {
    luaL_newmetatable(L, LUA_CRYPTO_AES_CBC_CTX_NAME);  /* metatable for AES CBC context */
    luaL_setfuncs(L, lcrypto_aes_cbc_ctx_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lcrypto_aes_cbc_ctx_meth);  /* create method table */
    luaL_setfuncs(L, lcrypto_aes_cbc_ctx_meth, 0);  /* add AES CBC context methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_crypto_aes(lua_State *L) {
    luaL_newlib(L, lcrypt_aes_funcs); /* new module */
    lcrypto_aes_createmeta(L);
    return 1;
}
