// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <mbedtls/md.h>
#include <pal/memory.h>
#include <pal/crypto/md.h>
#include <HAPBase.h>

struct pal_md_ctx {
    bool hmac;
    mbedtls_md_context_t ctx;
};

static const mbedtls_md_type_t pal_md_type_mapping[] = {
    [PAL_MD_MD4] = MBEDTLS_MD_MD4,
    [PAL_MD_MD5] = MBEDTLS_MD_MD5,
    [PAL_MD_SHA1] = MBEDTLS_MD_SHA1,
    [PAL_MD_SHA224] = MBEDTLS_MD_SHA224,
    [PAL_MD_SHA256] = MBEDTLS_MD_SHA256,
    [PAL_MD_SHA384] = MBEDTLS_MD_SHA384,
    [PAL_MD_SHA512] = MBEDTLS_MD_SHA512,
    [PAL_MD_RIPEMD160] = MBEDTLS_MD_RIPEMD160
};

pal_md_ctx *pal_md_new(pal_md_type type, const void *key, size_t len) {
    bool hmac = key != NULL;
    pal_md_ctx *ctx = pal_mem_alloc(sizeof(*ctx));
    if (!ctx) {
        return NULL;
    }
    ctx->hmac = hmac;
    mbedtls_md_init(&ctx->ctx);
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(pal_md_type_mapping[type]);
    if (!info) {
        goto err;
    }
    if (mbedtls_md_setup(&ctx->ctx, info, hmac)) {
        goto err;
    }
    int ret;
    if (hmac) {
        ret = mbedtls_md_hmac_starts(&ctx->ctx, key, len);
    } else {
        ret = mbedtls_md_starts(&ctx->ctx);
    }
    if (ret) {
        goto err;
    }
    return ctx;

err:
    pal_mem_free(ctx);
    return NULL;
}

void pal_md_free(pal_md_ctx *ctx) {
    if (ctx) {
        mbedtls_md_free(&ctx->ctx);
        pal_mem_free(ctx);
    }
}

size_t pal_md_get_size(pal_md_ctx *ctx) {
    HAPPrecondition(ctx);

    return mbedtls_md_get_size(ctx->ctx.md_info);
}

bool pal_md_update(pal_md_ctx *ctx, const void *data, size_t len) {
    HAPPrecondition(ctx);
    HAPPrecondition(data);
    HAPPrecondition(len > 0);

    if (ctx->hmac) {
        return mbedtls_md_hmac_update(&ctx->ctx, data, len) == 0;
    } else {
        return mbedtls_md_update(&ctx->ctx, data, len) == 0;
    }
}

bool pal_md_digest(pal_md_ctx *ctx, uint8_t *output) {
    HAPPrecondition(ctx);
    HAPPrecondition(output);

    if (ctx->hmac) {
        return mbedtls_md_hmac_finish(&ctx->ctx, output) == 0;
    } else {
        return mbedtls_md_finish(&ctx->ctx, output) == 0;
    }
}
