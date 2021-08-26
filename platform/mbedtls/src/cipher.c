// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <mbedtls/cipher.h>
#include <mbedtls/error.h>
#include <pal/cipher.h>
#include <pal/memory.h>
#include <HAPPlatform.h>

#define MBEDTLS_PRINT_ERROR(func, err) \
do { \
    char buf[128]; \
    mbedtls_strerror(err, buf, sizeof(buf)); \
    HAPLogError(&cipher_log_obj, \
        "%s: %s() returned -%04X: %s", __func__, #func, -err, buf); \
} while (0)

struct pal_cipher_ctx {
    mbedtls_cipher_context_t ctx;
    pal_cipher_operation op;
};

static const HAPLogObject cipher_log_obj = {
    .subsystem = kHAPPlatform_LogSubsystem,
    .category = "cipher",
};

static const mbedtls_cipher_type_t pal_cipher_mbedtls_types[] = {
    [PAL_CIPHER_TYPE_AES_128_ECB] = MBEDTLS_CIPHER_AES_128_ECB,
    [PAL_CIPHER_TYPE_AES_192_ECB] = MBEDTLS_CIPHER_AES_192_ECB,
    [PAL_CIPHER_TYPE_AES_256_ECB] = MBEDTLS_CIPHER_AES_256_ECB,
    [PAL_CIPHER_TYPE_AES_128_CBC] = MBEDTLS_CIPHER_AES_128_CBC,
    [PAL_CIPHER_TYPE_AES_192_CBC] = MBEDTLS_CIPHER_AES_192_CBC,
    [PAL_CIPHER_TYPE_AES_256_CBC] = MBEDTLS_CIPHER_AES_256_CBC,
    [PAL_CIPHER_TYPE_AES_128_CFB128] = MBEDTLS_CIPHER_AES_128_CFB128,
    [PAL_CIPHER_TYPE_AES_192_CFB128] = MBEDTLS_CIPHER_AES_192_CFB128,
    [PAL_CIPHER_TYPE_AES_256_CFB128] = MBEDTLS_CIPHER_AES_256_CFB128,
    [PAL_CIPHER_TYPE_AES_128_CTR] = MBEDTLS_CIPHER_AES_128_CTR,
    [PAL_CIPHER_TYPE_AES_192_CTR] = MBEDTLS_CIPHER_AES_192_CTR,
    [PAL_CIPHER_TYPE_AES_256_CTR] = MBEDTLS_CIPHER_AES_256_CTR,
    [PAL_CIPHER_TYPE_AES_128_GCM] = MBEDTLS_CIPHER_AES_128_GCM,
    [PAL_CIPHER_TYPE_AES_192_GCM] = MBEDTLS_CIPHER_AES_192_GCM,
    [PAL_CIPHER_TYPE_AES_256_GCM] = MBEDTLS_CIPHER_AES_256_GCM,
    [PAL_CIPHER_TYPE_CAMELLIA_128_ECB] = MBEDTLS_CIPHER_CAMELLIA_128_ECB,
    [PAL_CIPHER_TYPE_CAMELLIA_192_ECB] = MBEDTLS_CIPHER_CAMELLIA_192_ECB,
    [PAL_CIPHER_TYPE_CAMELLIA_256_ECB] = MBEDTLS_CIPHER_CAMELLIA_256_ECB,
    [PAL_CIPHER_TYPE_CAMELLIA_128_CBC] = MBEDTLS_CIPHER_CAMELLIA_128_CBC,
    [PAL_CIPHER_TYPE_CAMELLIA_192_CBC] = MBEDTLS_CIPHER_CAMELLIA_192_CBC,
    [PAL_CIPHER_TYPE_CAMELLIA_256_CBC] = MBEDTLS_CIPHER_CAMELLIA_256_CBC,
    [PAL_CIPHER_TYPE_CAMELLIA_128_CFB128] = MBEDTLS_CIPHER_CAMELLIA_128_CFB128,
    [PAL_CIPHER_TYPE_CAMELLIA_192_CFB128] = MBEDTLS_CIPHER_CAMELLIA_192_CFB128,
    [PAL_CIPHER_TYPE_CAMELLIA_256_CFB128] = MBEDTLS_CIPHER_CAMELLIA_256_CFB128,
    [PAL_CIPHER_TYPE_CAMELLIA_128_CTR] = MBEDTLS_CIPHER_CAMELLIA_128_CTR,
    [PAL_CIPHER_TYPE_CAMELLIA_192_CTR] = MBEDTLS_CIPHER_CAMELLIA_192_CTR,
    [PAL_CIPHER_TYPE_CAMELLIA_256_CTR] = MBEDTLS_CIPHER_CAMELLIA_256_CTR,
    [PAL_CIPHER_TYPE_CAMELLIA_128_GCM] = MBEDTLS_CIPHER_CAMELLIA_128_GCM,
    [PAL_CIPHER_TYPE_CAMELLIA_192_GCM] = MBEDTLS_CIPHER_CAMELLIA_192_GCM,
    [PAL_CIPHER_TYPE_CAMELLIA_256_GCM] = MBEDTLS_CIPHER_CAMELLIA_256_GCM,
    [PAL_CIPHER_TYPE_DES_ECB] = MBEDTLS_CIPHER_DES_ECB,
    [PAL_CIPHER_TYPE_DES_CBC] = MBEDTLS_CIPHER_DES_CBC,
    [PAL_CIPHER_TYPE_DES_EDE_ECB] = MBEDTLS_CIPHER_DES_EDE_ECB,
    [PAL_CIPHER_TYPE_DES_EDE_CBC] = MBEDTLS_CIPHER_DES_EDE_CBC,
    [PAL_CIPHER_TYPE_DES_EDE3_ECB] = MBEDTLS_CIPHER_DES_EDE3_ECB,
    [PAL_CIPHER_TYPE_DES_EDE3_CBC] = MBEDTLS_CIPHER_DES_EDE3_CBC,
    [PAL_CIPHER_TYPE_BLOWFISH_ECB] = MBEDTLS_CIPHER_BLOWFISH_ECB,
    [PAL_CIPHER_TYPE_BLOWFISH_CBC] = MBEDTLS_CIPHER_BLOWFISH_CBC,
    [PAL_CIPHER_TYPE_BLOWFISH_CFB64] = MBEDTLS_CIPHER_BLOWFISH_CFB64,
    [PAL_CIPHER_TYPE_BLOWFISH_CTR] = MBEDTLS_CIPHER_BLOWFISH_CTR,
    [PAL_CIPHER_TYPE_ARC4_128] = MBEDTLS_CIPHER_ARC4_128,
    [PAL_CIPHER_TYPE_AES_128_CCM] = MBEDTLS_CIPHER_AES_128_CCM,
    [PAL_CIPHER_TYPE_AES_192_CCM] = MBEDTLS_CIPHER_AES_192_CCM,
    [PAL_CIPHER_TYPE_AES_256_CCM] = MBEDTLS_CIPHER_AES_256_CCM,
    [PAL_CIPHER_TYPE_CAMELLIA_128_CCM] = MBEDTLS_CIPHER_CAMELLIA_128_CCM,
    [PAL_CIPHER_TYPE_CAMELLIA_192_CCM] = MBEDTLS_CIPHER_CAMELLIA_192_CCM,
    [PAL_CIPHER_TYPE_CAMELLIA_256_CCM] = MBEDTLS_CIPHER_CAMELLIA_256_CCM,
    [PAL_CIPHER_TYPE_ARIA_128_ECB] = MBEDTLS_CIPHER_ARIA_128_ECB,
    [PAL_CIPHER_TYPE_ARIA_192_ECB] = MBEDTLS_CIPHER_ARIA_192_ECB,
    [PAL_CIPHER_TYPE_ARIA_256_ECB] = MBEDTLS_CIPHER_ARIA_256_ECB,
    [PAL_CIPHER_TYPE_ARIA_128_CBC] = MBEDTLS_CIPHER_ARIA_128_CBC,
    [PAL_CIPHER_TYPE_ARIA_192_CBC] = MBEDTLS_CIPHER_ARIA_192_CBC,
    [PAL_CIPHER_TYPE_ARIA_256_CBC] = MBEDTLS_CIPHER_ARIA_256_CBC,
    [PAL_CIPHER_TYPE_ARIA_128_CFB128] = MBEDTLS_CIPHER_ARIA_128_CFB128,
    [PAL_CIPHER_TYPE_ARIA_192_CFB128] = MBEDTLS_CIPHER_ARIA_192_CFB128,
    [PAL_CIPHER_TYPE_ARIA_256_CFB128] = MBEDTLS_CIPHER_ARIA_256_CFB128,
    [PAL_CIPHER_TYPE_ARIA_128_CTR] = MBEDTLS_CIPHER_ARIA_128_CTR,
    [PAL_CIPHER_TYPE_ARIA_192_CTR] = MBEDTLS_CIPHER_ARIA_192_CTR,
    [PAL_CIPHER_TYPE_ARIA_256_CTR] = MBEDTLS_CIPHER_ARIA_256_CTR,
    [PAL_CIPHER_TYPE_ARIA_128_GCM] = MBEDTLS_CIPHER_ARIA_128_GCM,
    [PAL_CIPHER_TYPE_ARIA_192_GCM] = MBEDTLS_CIPHER_ARIA_192_GCM,
    [PAL_CIPHER_TYPE_ARIA_256_GCM] = MBEDTLS_CIPHER_ARIA_256_GCM,
    [PAL_CIPHER_TYPE_ARIA_128_CCM] = MBEDTLS_CIPHER_ARIA_128_CCM,
    [PAL_CIPHER_TYPE_ARIA_192_CCM] = MBEDTLS_CIPHER_ARIA_192_CCM,
    [PAL_CIPHER_TYPE_ARIA_256_CCM] = MBEDTLS_CIPHER_ARIA_256_CCM,
    [PAL_CIPHER_TYPE_AES_128_OFB] = MBEDTLS_CIPHER_AES_128_OFB,
    [PAL_CIPHER_TYPE_AES_192_OFB] = MBEDTLS_CIPHER_AES_192_OFB,
    [PAL_CIPHER_TYPE_AES_256_OFB] = MBEDTLS_CIPHER_AES_256_OFB,
    [PAL_CIPHER_TYPE_AES_128_XTS] = MBEDTLS_CIPHER_AES_128_XTS,
    [PAL_CIPHER_TYPE_AES_256_XTS] = MBEDTLS_CIPHER_AES_256_XTS,
    [PAL_CIPHER_TYPE_CHACHA20] = MBEDTLS_CIPHER_CHACHA20,
    [PAL_CIPHER_TYPE_CHACHA20_POLY1305] = MBEDTLS_CIPHER_CHACHA20_POLY1305,
};

static const mbedtls_cipher_padding_t pal_cipher_mbedtls_paddings[] = {
    [PAL_CIPHER_PADDING_NONE] = MBEDTLS_PADDING_NONE,
    [PAL_CIPHER_PADDING_PKCS7] = MBEDTLS_PADDING_PKCS7,
    [PAL_CIPHER_PADDING_ISO7816_4] = MBEDTLS_PADDING_ONE_AND_ZEROS,
    [PAL_CIPHER_PADDING_ANSI923] = MBEDTLS_PADDING_ZEROS_AND_LEN,
    [PAL_CIPHER_PADDING_ZERO] = MBEDTLS_PADDING_ZEROS,
};

static const mbedtls_operation_t pal_cipher_mbedtls_ops[] = {
    [PAL_CIPHER_OP_ENCRYPT] = MBEDTLS_ENCRYPT,
    [PAL_CIPHER_OP_DECRYPT] = MBEDTLS_DECRYPT,
};

pal_cipher_ctx *pal_cipher_new(pal_cipher_type type) {
    HAPPrecondition(type >= 0 && type < PAL_CIPHER_TYPE_MAX);
    pal_cipher_ctx *ctx = pal_mem_alloc(sizeof(*ctx));
    if (!ctx) {
        return NULL;
    }

    ctx->op = PAL_CIPHER_OP_NONE;
    mbedtls_cipher_init(&ctx->ctx);
    int ret = mbedtls_cipher_setup(&ctx->ctx,
        mbedtls_cipher_info_from_type(pal_cipher_mbedtls_types[type]));
    if (ret) {
        MBEDTLS_PRINT_ERROR(mbedtls_cipher_setup, ret);
        mbedtls_cipher_free(&ctx->ctx);
        pal_mem_free(ctx);
        return NULL;
    }
    return ctx;
}

void pal_cipher_free(pal_cipher_ctx *ctx) {
    if (!ctx) {
        return;
    }
    mbedtls_cipher_free(&ctx->ctx);
    pal_mem_free(ctx);
}

size_t pal_cipher_get_block_size(pal_cipher_ctx *ctx) {
    HAPPrecondition(ctx);

    return mbedtls_cipher_get_block_size(&ctx->ctx);
}

size_t pal_cipher_get_key_len(pal_cipher_ctx *ctx) {
    HAPPrecondition(ctx);

    return mbedtls_cipher_get_key_bitlen(&ctx->ctx) / 8;
}

size_t pal_cipher_get_iv_len(pal_cipher_ctx *ctx) {
    HAPPrecondition(ctx);

    return mbedtls_cipher_get_iv_size(&ctx->ctx);
}

bool pal_cipher_set_padding(pal_cipher_ctx *ctx, pal_cipher_padding padding) {
    HAPPrecondition(ctx);
    HAPPrecondition(padding >= PAL_CIPHER_PADDING_NONE &&
        padding < PAL_CIPHER_PADDING_MAX);

    int ret = mbedtls_cipher_set_padding_mode(&ctx->ctx, pal_cipher_mbedtls_paddings[padding]);
    if (ret) {
        MBEDTLS_PRINT_ERROR(mbedtls_cipher_set_padding_mode, ret);
        return false;
    }
    return true;
}

bool pal_cipher_begin(pal_cipher_ctx *ctx, pal_cipher_operation op, const uint8_t *key, const uint8_t *iv) {
    HAPPrecondition(ctx);
    HAPPrecondition(ctx->op == PAL_CIPHER_OP_NONE);
    HAPPrecondition(op > PAL_CIPHER_OP_NONE && op < PAL_CIPHER_OP_MAX);

    if (mbedtls_cipher_set_iv(&ctx->ctx, iv, mbedtls_cipher_get_iv_size(&ctx->ctx))) {
        return false;
    }
    int ret = mbedtls_cipher_setkey(&ctx->ctx, key,
        mbedtls_cipher_get_key_bitlen(&ctx->ctx), pal_cipher_mbedtls_ops[op]);
    if (ret) {
        MBEDTLS_PRINT_ERROR(mbedtls_cipher_setkey, ret);
        return false;
    }
    mbedtls_cipher_reset(&ctx->ctx);
    ctx->op = op;
    return true;
}

bool pal_cipher_update(pal_cipher_ctx *ctx, const void *in, size_t ilen, void *out, size_t *olen) {
    HAPPrecondition(ctx);
    HAPPrecondition(ctx->op != PAL_CIPHER_OP_NONE);
    HAPPrecondition(in);
    HAPPrecondition(out);
    HAPPrecondition(ilen > 0);
    HAPPrecondition(olen);
    HAPPrecondition(*olen >= ilen + pal_cipher_get_block_size(ctx));

    int ret = mbedtls_cipher_update(&ctx->ctx, in, ilen, out, olen);
    if (ret) {
        MBEDTLS_PRINT_ERROR(mbedtls_cipher_update, ret);
        return false;
    }
    return true;
}

bool pal_cipher_finsh(pal_cipher_ctx *ctx, void *out, size_t *olen) {
    HAPPrecondition(ctx);
    HAPPrecondition(ctx->op != PAL_CIPHER_OP_NONE);
    HAPPrecondition(out);
    HAPPrecondition(olen);
    HAPPrecondition(*olen >= pal_cipher_get_block_size(ctx));

    int ret = mbedtls_cipher_finish(&ctx->ctx, out, olen);
    if (ret) {
        MBEDTLS_PRINT_ERROR(mbedtls_cipher_finish, ret);
        return false;
    }
    ctx->op = PAL_CIPHER_OP_NONE;
    return true;
}
