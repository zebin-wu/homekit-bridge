// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <mbedtls/aes.h>
#include <pal/crypto/aes.h>
#include <pal/memory.h>

struct pal_crypto_aes_ctx {
    mbedtls_aes_context ctx;
};

pal_crypto_aes_ctx *pal_crypto_aes_new(void) {
    return pal_mem_alloc(sizeof(pal_crypto_aes_ctx));
}

void pal_crypto_aes_free(pal_crypto_aes_ctx *ctx) {
    pal_mem_free(ctx);
}

bool pal_crypto_aes_set_enc_key(pal_crypto_aes_ctx *ctx, const void *key, size_t len) {
    HAPPrecondition(ctx);
    HAPPrecondition(key);
    HAPPrecondition(len == 16 || len == 24 || len == 32);
    return mbedtls_aes_setkey_enc(&ctx->ctx, key, len * 8) == 0;
}

bool pal_crypto_aes_set_dec_key(pal_crypto_aes_ctx *ctx, const void *key, size_t len) {
    HAPPrecondition(ctx);
    HAPPrecondition(key);
    HAPPrecondition(len == 16 || len == 24 || len == 32);
    return mbedtls_aes_setkey_dec(&ctx->ctx, key, len * 8) == 0;
}

void pal_crypto_aes_cbc_enc(pal_crypto_aes_ctx *ctx, uint8_t iv[16], const void *in, void *out, size_t len) {
    HAPPrecondition(ctx);
    HAPPrecondition(iv);
    HAPPrecondition(in);
    HAPPrecondition(out);
    HAPPrecondition(len % 16 == 0);
    mbedtls_aes_crypt_cbc(&ctx->ctx, MBEDTLS_AES_ENCRYPT, len, iv, in, out);
}

void pal_crypto_aes_cbc_dec(pal_crypto_aes_ctx *ctx, uint8_t iv[16], const void *in, void *out, size_t len) {
    HAPPrecondition(ctx);
    HAPPrecondition(iv);
    HAPPrecondition(in);
    HAPPrecondition(out);
    HAPPrecondition(len % 16 == 0);
    mbedtls_aes_crypt_cbc(&ctx->ctx, MBEDTLS_AES_DECRYPT, len, iv, in, out);
}
