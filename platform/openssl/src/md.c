// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <pal/memory.h>
#include <pal/crypto/md.h>
#include <HAPBase.h>

struct pal_md_ctx {
    bool hmac;
    void *ctx;
};

static const EVP_MD *pal_md_get_md(pal_md_type type) {
    const EVP_MD *(*pal_cipher_md_funcs[])(void) = {
        [PAL_MD_MD4] = EVP_md4,
        [PAL_MD_MD5] = EVP_md5,
        [PAL_MD_SHA1] = EVP_sha1,
        [PAL_MD_SHA224] = EVP_sha224,
        [PAL_MD_SHA256] = EVP_sha256,
        [PAL_MD_SHA384] = EVP_sha384,
        [PAL_MD_SHA512] = EVP_sha512,
        [PAL_MD_RIPEMD160] = EVP_ripemd160,
    };

    if (!pal_cipher_md_funcs[type]) {
        return NULL;
    }

    return pal_cipher_md_funcs[type]();
}

pal_md_ctx *pal_md_new(pal_md_type type, const void *key, size_t len) {
    if (key) {
        HAPPrecondition(len > 0);
    }
    bool hmac = key != NULL;
    pal_md_ctx *ctx = pal_mem_alloc(sizeof(*ctx));
    if (!ctx) {
        return NULL;
    }
    ctx->hmac = hmac;
    if (hmac) {
        ctx->ctx = HMAC_CTX_new();
    } else {
        ctx->ctx = EVP_MD_CTX_new();
    }
    if (!ctx->ctx) {
        goto err;
    }
    const EVP_MD *md = pal_md_get_md(type);
    if (!md) {
        goto err1;
    }

    int ret;
    if (hmac) {
        ret = HMAC_Init_ex(ctx->ctx, key, len, md, NULL);
    } else {
        ret = EVP_DigestInit(ctx->ctx, md);
    }
    if (!ret) {
        goto err1;
    }
    return ctx;

err1:
    if (hmac) {
        HMAC_CTX_free(ctx->ctx);
    } else {
        EVP_MD_CTX_free(ctx->ctx);
    }
err:
    pal_mem_free(ctx);
    return NULL;
}

void pal_md_free(pal_md_ctx *ctx) {
    if (ctx) {
        if (ctx->hmac) {
            HMAC_CTX_free(ctx->ctx);
        } else {
            EVP_MD_CTX_free(ctx->ctx);
        }
        pal_mem_free(ctx);
    }
}

size_t pal_md_get_size(pal_md_ctx *ctx) {
    HAPPrecondition(ctx);

    return EVP_MD_size(ctx->hmac ? HMAC_CTX_get_md(ctx->ctx) : EVP_MD_CTX_md(ctx->ctx));
}

bool pal_md_update(pal_md_ctx *ctx, const void *data, size_t len) {
    HAPPrecondition(ctx);
    HAPPrecondition(data);
    HAPPrecondition(len > 0);

    if (ctx->hmac) {
        return HMAC_Update(ctx->ctx, data, len);
    } else {
        return EVP_DigestUpdate(ctx->ctx, data, len);
    }
}

bool pal_md_digest(pal_md_ctx *ctx, uint8_t *output) {
    HAPPrecondition(ctx);
    HAPPrecondition(output);

    if (ctx->hmac) {
        return HMAC_Final(ctx->ctx, output, NULL);
    } else {
        return EVP_DigestFinal(ctx->ctx, output, NULL);
    }
}
