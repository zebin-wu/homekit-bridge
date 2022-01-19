// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <mbedtls/md5.h>
#include <pal/memory.h>
#include <pal/md5.h>
#include <HAPBase.h>

struct pal_md5_ctx {
    mbedtls_md5_context ctx;
};

pal_md5_ctx *pal_md5_new(void) {
    pal_md5_ctx *ctx = pal_mem_alloc(sizeof(*ctx));
    if (!ctx) {
        return NULL;
    }

    mbedtls_md5_init(&ctx->ctx);
    mbedtls_md5_starts_ret(&ctx->ctx);

    return ctx;
}

void pal_md5_free(pal_md5_ctx *ctx) {
    if (ctx) {
        pal_mem_free(ctx);
    }
}

void pal_md5_update(pal_md5_ctx *ctx, const void *data, size_t len) {
    HAPPrecondition(ctx);
    HAPPrecondition(data);
    HAPPrecondition(len > 0);

    HAPAssert(mbedtls_md5_update_ret(&ctx->ctx, data, len) == 0);
}

void pal_md5_digest(pal_md5_ctx *ctx, uint8_t output[PAL_MD5_HASHSIZE]) {
    HAPPrecondition(ctx);
    HAPPrecondition(output);

    HAPAssert(mbedtls_md5_finish_ret(&ctx->ctx, output) == 0);
}
