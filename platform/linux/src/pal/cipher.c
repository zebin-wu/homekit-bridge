// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <openssl/evp.h>
#include <pal/cipher.h>
#include <pal/memory.h>

struct pal_cipher_ctx {
    EVP_CIPHER_CTX *ctx;
    const EVP_CIPHER *cipher;
    pal_cipher_operation op;
};

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
    const EVP_CIPHER *cipher = NULL;

    switch (type) {
    case PAL_CIPHER_TYPE_AES_128_CBC:
        cipher = EVP_aes_128_cbc();
        break;
    default:
        break;
    }
    return cipher;
}

pal_cipher_ctx *pal_cipher_new(pal_cipher_type type) {
    HAPPrecondition(type > PAL_CIPHER_TYPE_NONE && type < PAL_CIPHER_TYPE_MAX);
    pal_cipher_ctx *ctx = pal_mem_alloc(sizeof(*ctx));
    if (!ctx) {
        return NULL;
    }
    ctx->ctx = EVP_CIPHER_CTX_new();
    if (!ctx->ctx) {
        goto err;
    }
    ctx->op = PAL_CIPHER_OP_NONE;
    ctx->cipher = pal_cipher_get_cipher(type);
    if (!ctx->cipher) {
        goto err;
    }
    pal_cipher_set_padding(ctx, PAL_CIPHER_PADDING_NONE);
    return ctx;

err:
    pal_mem_free(ctx);
    return NULL;
}

void pal_cipher_free(pal_cipher_ctx *ctx) {
    if (!ctx)  {
        return;
    }
    EVP_CIPHER_CTX_free(ctx->ctx);
    pal_mem_free(ctx);
}

bool pal_cipher_reset(pal_cipher_ctx *ctx) {
    HAPPrecondition(ctx);

    ctx->op = PAL_CIPHER_OP_NONE;
    pal_cipher_set_padding(ctx, PAL_CIPHER_PADDING_NONE);
    return EVP_CIPHER_CTX_reset(ctx->ctx);
}

size_t pal_cipher_get_block_size(pal_cipher_ctx *ctx) {
    HAPPrecondition(ctx);

    return EVP_CIPHER_block_size(ctx->cipher);
}

size_t pal_cipher_get_key_len(pal_cipher_ctx *ctx) {
    HAPPrecondition(ctx);

    return EVP_CIPHER_key_length(ctx->cipher);
}

size_t pal_cipher_get_iv_len(pal_cipher_ctx *ctx) {
    HAPPrecondition(ctx);

    return EVP_CIPHER_iv_length(ctx->cipher);
}

bool pal_cipher_set_padding(pal_cipher_ctx *ctx, pal_cipher_padding padding) {
    HAPPrecondition(ctx);
    HAPPrecondition(padding >= PAL_CIPHER_PADDING_NONE &&
        padding < PAL_CIPHER_PADDING_MAX);

    return EVP_CIPHER_CTX_set_padding(ctx->ctx, pal_cipher_openssl_paddings[padding]);
}

bool pal_cipher_begin(pal_cipher_ctx *ctx, pal_cipher_operation op, const uint8_t *key, const uint8_t *iv) {
    HAPPrecondition(ctx);
    HAPPrecondition(ctx->op == PAL_CIPHER_OP_NONE);
    HAPPrecondition(op > PAL_CIPHER_OP_NONE && op < PAL_CIPHER_OP_MAX);

    ctx->op = op;
    return pal_cipher_crypt_funcs[ctx->op].init(ctx->ctx, ctx->cipher, NULL, key, iv);
}

bool pal_cipher_update(pal_cipher_ctx *ctx, const void *in, size_t ilen, void *out, size_t *olen) {
    HAPPrecondition(ctx);
    HAPPrecondition(ctx->op != PAL_CIPHER_OP_NONE);
    HAPPrecondition(in);
    HAPPrecondition(out);

    return pal_cipher_crypt_funcs[ctx->op].update(ctx->ctx, out, (int *)olen, in, ilen);
}

bool pal_cipher_finsh(pal_cipher_ctx *ctx, void *out, size_t *olen) {
    HAPPrecondition(ctx);
    HAPPrecondition(ctx->op != PAL_CIPHER_OP_NONE);
    HAPPrecondition(out);

    ctx->op = PAL_CIPHER_OP_NONE;
    return pal_cipher_crypt_funcs[ctx->op].final(ctx->ctx, out, (int *)olen);
}
