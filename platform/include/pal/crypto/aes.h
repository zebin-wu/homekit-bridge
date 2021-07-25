// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_CRYPTO_AES_H_
#define PLATFORM_INCLUDE_PAL_CRYPTO_AES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <HAPBase.h>

typedef struct pal_crypto_aes_ctx pal_crypto_aes_ctx;

/**
 * New A AES context.
 */
pal_crypto_aes_ctx *pal_crypto_aes_new(void);

/**
 * Free A AES context.
 *
 * @param ctx The AES context.
 */
void pal_crypto_aes_free(pal_crypto_aes_ctx *ctx);

/**
 * Set encryption key.
 *
 * @param ctx The AES context.
 * @param key The encryption key.
 * @param len The bytes of data passed. Valid options are: 16, 24, 32.
 */
bool pal_crypto_aes_set_enc_key(pal_crypto_aes_ctx *ctx, const void *key, size_t len);

/**
 * Set decryption key.
 *
 * @param ctx The AES context.
 * @param key The encryption key.
 * @param len The bytes of data passed. Valid options are: 16, 24, 32.
 */
bool pal_crypto_aes_set_dec_key(pal_crypto_aes_ctx *ctx, const void *key, size_t len);

/**
 * Encrypt a block which size is a multiple of the AES block size of 16 Bytes.
 */
void pal_crypto_aes_cbc_enc(pal_crypto_aes_ctx *ctx, uint8_t iv[16], const void *in, void *out, size_t len);

/**
 * Decrypt a block which size is a multiple of the AES block size of 16 Bytes.
 */
void pal_crypto_aes_cbc_dec(pal_crypto_aes_ctx *ctx, uint8_t iv[16], const void *in, void *out, size_t len);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_CRYPTO_AES_H_
