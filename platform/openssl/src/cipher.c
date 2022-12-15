// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <openssl/evp.h>
#include <pal/cipher.h>
#include <HAPBase.h>

typedef struct pal_cipher_ctx_int {
    EVP_CIPHER_CTX *ctx;
    const EVP_CIPHER *cipher;
    pal_cipher_padding padding;
    pal_cipher_operation op;
} pal_cipher_ctx_int;
HAP_STATIC_ASSERT(sizeof(pal_cipher_ctx) >= sizeof(pal_cipher_ctx_int), pal_cipher_ctx_int);

static const struct {
    int (*init)(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher, ENGINE *impl,
            const unsigned char *key, const unsigned char *iv);
    int (*update)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
            const unsigned char *in, int inl);
    int (*final)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);
} pal_cipher_crypt_funcs[] = {
    [PAL_CIPHER_OP_ENCRYPT] = {
        EVP_EncryptInit_ex,
        EVP_EncryptUpdate,
        EVP_EncryptFinal_ex,
    },
    [PAL_CIPHER_OP_DECRYPT] = {
        EVP_DecryptInit_ex,
        EVP_DecryptUpdate,
        EVP_DecryptFinal_ex,
    },
};

static const int pal_cipher_openssl_paddings[] = {
    [PAL_CIPHER_PADDING_NONE] = 0,
    [PAL_CIPHER_PADDING_PKCS7] = EVP_PADDING_PKCS7,
    [PAL_CIPHER_PADDING_ISO7816_4] = EVP_PADDING_ISO7816_4,
    [PAL_CIPHER_PADDING_ANSI923] = EVP_PADDING_ANSI923,
    [PAL_CIPHER_PADDING_ZERO] = EVP_PADDING_ZERO,
};

static const EVP_CIPHER *pal_cipher_get_cipher(pal_cipher_type type) {
    const EVP_CIPHER *(*pal_cipher_get_cipher_funcs[])(void) = {
        [PAL_CIPHER_TYPE_AES_128_ECB] = EVP_aes_128_ecb,
        [PAL_CIPHER_TYPE_AES_192_ECB] = EVP_aes_192_ecb,
        [PAL_CIPHER_TYPE_AES_256_ECB] = EVP_aes_256_ecb,
        [PAL_CIPHER_TYPE_AES_128_CBC] = EVP_aes_128_cbc,
        [PAL_CIPHER_TYPE_AES_192_CBC] = EVP_aes_192_cbc,
        [PAL_CIPHER_TYPE_AES_256_CBC] = EVP_aes_256_cbc,
        [PAL_CIPHER_TYPE_AES_128_CFB128] = EVP_aes_128_cfb128,
        [PAL_CIPHER_TYPE_AES_192_CFB128] = EVP_aes_192_cfb128,
        [PAL_CIPHER_TYPE_AES_256_CFB128] = EVP_aes_256_cfb128,
        [PAL_CIPHER_TYPE_AES_128_CTR] = EVP_aes_128_ctr,
        [PAL_CIPHER_TYPE_AES_192_CTR] = EVP_aes_192_ctr,
        [PAL_CIPHER_TYPE_AES_256_CTR] = EVP_aes_256_ctr,
        [PAL_CIPHER_TYPE_AES_128_GCM] = EVP_aes_128_gcm,
        [PAL_CIPHER_TYPE_AES_192_GCM] = EVP_aes_192_gcm,
        [PAL_CIPHER_TYPE_AES_256_GCM] = EVP_aes_256_gcm,
        [PAL_CIPHER_TYPE_CAMELLIA_128_ECB] = EVP_camellia_128_ecb,
        [PAL_CIPHER_TYPE_CAMELLIA_192_ECB] = EVP_camellia_192_ecb,
        [PAL_CIPHER_TYPE_CAMELLIA_256_ECB] = EVP_camellia_256_ecb,
        [PAL_CIPHER_TYPE_CAMELLIA_128_CBC] = EVP_camellia_128_cbc,
        [PAL_CIPHER_TYPE_CAMELLIA_192_CBC] = EVP_camellia_192_cbc,
        [PAL_CIPHER_TYPE_CAMELLIA_256_CBC] = EVP_camellia_256_cbc,
        [PAL_CIPHER_TYPE_CAMELLIA_128_CFB128] = EVP_camellia_128_cfb128,
        [PAL_CIPHER_TYPE_CAMELLIA_192_CFB128] = EVP_camellia_192_cfb128,
        [PAL_CIPHER_TYPE_CAMELLIA_256_CFB128] = EVP_camellia_256_cfb128,
        [PAL_CIPHER_TYPE_CAMELLIA_128_CTR] = EVP_camellia_128_ctr,
        [PAL_CIPHER_TYPE_CAMELLIA_192_CTR] = EVP_camellia_192_ctr,
        [PAL_CIPHER_TYPE_CAMELLIA_256_CTR] = EVP_camellia_256_ctr,
        [PAL_CIPHER_TYPE_DES_ECB] = EVP_des_ecb,
        [PAL_CIPHER_TYPE_DES_CBC] = EVP_des_cbc,
        [PAL_CIPHER_TYPE_DES_EDE_ECB] = EVP_des_ede_ecb,
        [PAL_CIPHER_TYPE_DES_EDE_CBC] = EVP_des_ede_cbc,
        [PAL_CIPHER_TYPE_DES_EDE3_ECB] = EVP_des_ede3_ecb,
        [PAL_CIPHER_TYPE_DES_EDE3_CBC] = EVP_des_ede3_cbc,
        [PAL_CIPHER_TYPE_AES_128_CCM] = EVP_aes_128_ccm,
        [PAL_CIPHER_TYPE_AES_192_CCM] = EVP_aes_192_ccm,
        [PAL_CIPHER_TYPE_AES_256_CCM] = EVP_aes_256_ccm,
        [PAL_CIPHER_TYPE_ARIA_128_ECB] = EVP_aria_128_ecb,
        [PAL_CIPHER_TYPE_ARIA_192_ECB] = EVP_aria_192_ecb,
        [PAL_CIPHER_TYPE_ARIA_256_ECB] = EVP_aria_256_ecb,
        [PAL_CIPHER_TYPE_ARIA_128_CBC] = EVP_aria_128_cbc,
        [PAL_CIPHER_TYPE_ARIA_192_CBC] = EVP_aria_192_cbc,
        [PAL_CIPHER_TYPE_ARIA_256_CBC] = EVP_aria_256_cbc,
        [PAL_CIPHER_TYPE_ARIA_128_CFB128] = EVP_aria_128_cfb128,
        [PAL_CIPHER_TYPE_ARIA_192_CFB128] = EVP_aria_192_cfb128,
        [PAL_CIPHER_TYPE_ARIA_256_CFB128] = EVP_aria_256_cfb128,
        [PAL_CIPHER_TYPE_ARIA_128_CTR] = EVP_aria_128_ctr,
        [PAL_CIPHER_TYPE_ARIA_192_CTR] = EVP_aria_192_ctr,
        [PAL_CIPHER_TYPE_ARIA_256_CTR] = EVP_aria_256_ctr,
        [PAL_CIPHER_TYPE_ARIA_128_GCM] = EVP_aria_128_gcm,
        [PAL_CIPHER_TYPE_ARIA_192_GCM] = EVP_aria_192_gcm,
        [PAL_CIPHER_TYPE_ARIA_256_GCM] = EVP_aria_256_gcm,
        [PAL_CIPHER_TYPE_ARIA_128_CCM] = EVP_aria_128_ccm,
        [PAL_CIPHER_TYPE_ARIA_192_CCM] = EVP_aria_192_ccm,
        [PAL_CIPHER_TYPE_ARIA_256_CCM] = EVP_aria_256_ccm,
        [PAL_CIPHER_TYPE_AES_128_OFB] = EVP_aes_128_ofb,
        [PAL_CIPHER_TYPE_AES_192_OFB] = EVP_aes_192_ofb,
        [PAL_CIPHER_TYPE_AES_256_OFB] = EVP_aes_256_ofb,
        [PAL_CIPHER_TYPE_AES_128_XTS] = EVP_aes_128_xts,
        [PAL_CIPHER_TYPE_AES_256_XTS] = EVP_aes_256_xts,
        [PAL_CIPHER_TYPE_CHACHA20] = EVP_chacha20,
        [PAL_CIPHER_TYPE_CHACHA20_POLY1305] = EVP_chacha20_poly1305,
    };

    if (!pal_cipher_get_cipher_funcs[type]) {
        return NULL;
    }

    return pal_cipher_get_cipher_funcs[type]();
}

bool pal_cipher_ctx_init(pal_cipher_ctx *_ctx, pal_cipher_type type) {
    HAPPrecondition(_ctx);
    pal_cipher_ctx_int *ctx = (pal_cipher_ctx_int *)_ctx;
    HAPPrecondition(type >= 0 && type < PAL_CIPHER_TYPE_MAX);

    ctx->ctx = EVP_CIPHER_CTX_new();
    if (!ctx->ctx) {
        return false;
    }
    ctx->op = PAL_CIPHER_OP_NONE;
    ctx->cipher = pal_cipher_get_cipher(type);
    if (!ctx->cipher) {
        EVP_CIPHER_CTX_free(ctx->ctx);
        return false;
    }
    ctx->padding = PAL_CIPHER_PADDING_NONE;
    return true;
}

void pal_cipher_ctx_deinit(pal_cipher_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_cipher_ctx_int *ctx = (pal_cipher_ctx_int *)_ctx;
    EVP_CIPHER_CTX_free(ctx->ctx);
}

size_t pal_cipher_get_block_size(pal_cipher_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_cipher_ctx_int *ctx = (pal_cipher_ctx_int *)_ctx;
    return EVP_CIPHER_block_size(ctx->cipher);
}

size_t pal_cipher_get_key_len(pal_cipher_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_cipher_ctx_int *ctx = (pal_cipher_ctx_int *)_ctx;
    return EVP_CIPHER_key_length(ctx->cipher);
}

size_t pal_cipher_get_iv_len(pal_cipher_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_cipher_ctx_int *ctx = (pal_cipher_ctx_int *)_ctx;
    return EVP_CIPHER_iv_length(ctx->cipher);
}

bool pal_cipher_set_padding(pal_cipher_ctx *_ctx, pal_cipher_padding padding) {
    HAPPrecondition(_ctx);
    pal_cipher_ctx_int *ctx = (pal_cipher_ctx_int *)_ctx;
    HAPPrecondition(padding >= PAL_CIPHER_PADDING_NONE &&
        padding < PAL_CIPHER_PADDING_MAX);
    ctx->padding = padding;
    return true;
}

bool pal_cipher_begin(pal_cipher_ctx *_ctx, pal_cipher_operation op, const uint8_t *key, const uint8_t *iv) {
    HAPPrecondition(_ctx);
    pal_cipher_ctx_int *ctx = (pal_cipher_ctx_int *)_ctx;
    HAPPrecondition(ctx->op == PAL_CIPHER_OP_NONE);
    HAPPrecondition(op > PAL_CIPHER_OP_NONE && op < PAL_CIPHER_OP_MAX);

    bool status = pal_cipher_crypt_funcs[op].init(ctx->ctx, ctx->cipher, NULL, key, iv);
    if (status) {
        ctx->op = op;
        HAPAssert(EVP_CIPHER_CTX_set_padding(ctx->ctx, pal_cipher_openssl_paddings[ctx->padding]));
    }
    return status;
}

bool pal_cipher_update(pal_cipher_ctx *_ctx, const void *in, size_t ilen, void *out, size_t *olen) {
    HAPPrecondition(_ctx);
    pal_cipher_ctx_int *ctx = (pal_cipher_ctx_int *)_ctx;
    HAPPrecondition(ctx->op != PAL_CIPHER_OP_NONE);
    HAPPrecondition(in);
    HAPPrecondition(ilen > 0);
    HAPPrecondition(out);
    HAPPrecondition(olen);
    HAPPrecondition(*olen >= ilen + pal_cipher_get_block_size(_ctx));

    return pal_cipher_crypt_funcs[ctx->op].update(ctx->ctx, out, (int *)olen, in, ilen);
}

bool pal_cipher_finish(pal_cipher_ctx *_ctx, void *out, size_t *olen) {
    HAPPrecondition(_ctx);
    pal_cipher_ctx_int *ctx = (pal_cipher_ctx_int *)_ctx;
    HAPPrecondition(ctx->op != PAL_CIPHER_OP_NONE);
    HAPPrecondition(out);
    HAPPrecondition(olen);
    HAPPrecondition(*olen >= pal_cipher_get_block_size(_ctx));

    bool status = pal_cipher_crypt_funcs[ctx->op].final(ctx->ctx, out, (int *)olen);
    if (status) {
        ctx->op = PAL_CIPHER_OP_NONE;
    }
    return status;
}
