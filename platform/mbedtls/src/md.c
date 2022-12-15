// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <mbedtls/md.h>
#include <pal/md.h>
#include <HAPBase.h>

typedef struct pal_md_ctx_int {
    bool hmac;
    mbedtls_md_context_t ctx;
} pal_md_ctx_int;
HAP_STATIC_ASSERT(sizeof(pal_md_ctx) >= sizeof(pal_md_ctx_int), pal_md_ctx_int);

static const mbedtls_md_type_t pal_md_type_mapping[] = {
    [PAL_MD_MD5] = MBEDTLS_MD_MD5,
    [PAL_MD_SHA1] = MBEDTLS_MD_SHA1,
    [PAL_MD_SHA224] = MBEDTLS_MD_SHA224,
    [PAL_MD_SHA256] = MBEDTLS_MD_SHA256,
    [PAL_MD_SHA384] = MBEDTLS_MD_SHA384,
    [PAL_MD_SHA512] = MBEDTLS_MD_SHA512,
    [PAL_MD_RIPEMD160] = MBEDTLS_MD_RIPEMD160
};

bool pal_md_ctx_init(pal_md_ctx *_ctx, pal_md_type type, const void *key, size_t len) {
    HAPPrecondition(_ctx);
    HAPPrecondition(type >= 0 && type < PAL_MD_TYPE_MAX);
    HAPPrecondition((key && len) || (!key && !len));

    pal_md_ctx_int *ctx = (pal_md_ctx_int *)_ctx;
    ctx->hmac = key != NULL;
    mbedtls_md_init(&ctx->ctx);
    if (mbedtls_md_setup(&ctx->ctx,
        mbedtls_md_info_from_type(pal_md_type_mapping[type]), ctx->hmac)) {
        return false;
    }
    int ret;
    if (ctx->hmac) {
        ret = mbedtls_md_hmac_starts(&ctx->ctx, key, len);
    } else {
        ret = mbedtls_md_starts(&ctx->ctx);
    }
    if (ret) {
        mbedtls_md_free(&ctx->ctx);
        return false;
    }
    return true;
}

void pal_md_ctx_deinit(pal_md_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_md_ctx_int *ctx = (pal_md_ctx_int *)_ctx;

    mbedtls_md_free(&ctx->ctx);
}

size_t pal_md_get_size(pal_md_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_md_ctx_int *ctx = (pal_md_ctx_int *)_ctx;

    return mbedtls_md_get_size(mbedtls_md_info_from_ctx(&ctx->ctx));
}

bool pal_md_update(pal_md_ctx *_ctx, const void *data, size_t len) {
    HAPPrecondition(_ctx);
    pal_md_ctx_int *ctx = (pal_md_ctx_int *)_ctx;
    HAPPrecondition(data);
    HAPPrecondition(len > 0);

    if (ctx->hmac) {
        return mbedtls_md_hmac_update(&ctx->ctx, data, len) == 0;
    } else {
        return mbedtls_md_update(&ctx->ctx, data, len) == 0;
    }
}

bool pal_md_digest(pal_md_ctx *_ctx, uint8_t *output) {
    HAPPrecondition(_ctx);
    pal_md_ctx_int *ctx = (pal_md_ctx_int *)_ctx;
    HAPPrecondition(output);

    if (ctx->hmac) {
        return mbedtls_md_hmac_finish(&ctx->ctx, output) == 0;
    } else {
        return mbedtls_md_finish(&ctx->ctx, output) == 0;
    }
}
