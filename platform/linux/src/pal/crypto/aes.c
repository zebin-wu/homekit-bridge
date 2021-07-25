// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <openssl/aes.h>
#include <openssl/modes.h>
#include <pal/crypto/aes.h>
#include <pal/memory.h>

struct pal_crypto_aes_ctx {
    AES_KEY key;
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
    return AES_set_encrypt_key(key, len * 8, &ctx->key) == 0;
}

bool pal_crypto_aes_set_dec_key(pal_crypto_aes_ctx *ctx, const void *key, size_t len) {
    HAPPrecondition(ctx);
    HAPPrecondition(key);
    HAPPrecondition(len == 16 || len == 24 || len == 32);
    return AES_set_decrypt_key(key, len * 8, &ctx->key) == 0;
}

void pal_crypto_aes_cbc_enc(pal_crypto_aes_ctx *ctx, uint8_t iv[16], const void *in, void *out, size_t len) {
    HAPPrecondition(ctx);
    HAPPrecondition(iv);
    HAPPrecondition(in);
    HAPPrecondition(out);
    HAPPrecondition(len % 16 == 0);
    CRYPTO_cbc128_encrypt(in, out, len, &ctx->key, iv, (block128_f)AES_encrypt);
}

void pal_crypto_aes_cbc_dec(pal_crypto_aes_ctx *ctx, uint8_t iv[16], const void *in, void *out, size_t len) {
    HAPPrecondition(ctx);
    HAPPrecondition(iv);
    HAPPrecondition(in);
    HAPPrecondition(out);
    HAPPrecondition(len % 16 == 0);
    CRYPTO_cbc128_decrypt(in, out, len, &ctx->key, iv, (block128_f)AES_decrypt);
}
