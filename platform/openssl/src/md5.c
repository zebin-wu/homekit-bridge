// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <openssl/md5.h>
#include <pal/memory.h>
#include <pal/md5.h>

struct pal_md5_ctx {
    MD5_CTX ctx;
};

pal_md5_ctx *pal_md5_new(void) {
    pal_md5_ctx *ctx = pal_mem_alloc(sizeof(*ctx));
    if (!ctx) {
        return NULL;
    }
    if (MD5_Init(&ctx->ctx) != true) {
        pal_mem_free(ctx);
        return NULL;
    }
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

    HAPAssert(MD5_Update(&ctx->ctx, data, len) == true);
}

void pal_md5_digest(pal_md5_ctx *ctx, uint8_t output[PAL_MD5_HASHSIZE]) {
    HAPPrecondition(ctx);
    HAPPrecondition(output);

    HAPAssert(MD5_Final(output, &ctx->ctx) == true);
}
