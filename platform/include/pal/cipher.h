// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_CIPHER_H_
#define PLATFORM_INCLUDE_PAL_CIPHER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <HAPBase.h>

typedef enum {
    PAL_CIPHER_TYPE_NONE,
    PAL_CIPHER_TYPE_AES_128_CBC,

    PAL_CIPHER_TYPE_MAX,
} pal_cipher_type;

typedef enum {
    PAL_CIPHER_PADDING_NONE,          /**< Never pad (full blocks only).   */
    PAL_CIPHER_PADDING_PKCS7,         /**< PKCS7 padding (default).        */
    PAL_CIPHER_PADDING_ISO7816_4,     /**< ISO/IEC 7816-4 padding.         */
    PAL_CIPHER_PADDING_ANSI923,       /**< ANSI X.923 padding.             */
    PAL_CIPHER_PADDING_ZERO,          /**< Zero padding (not reversible).  */
    PAL_CIPHER_PADDING_MAX,
} pal_cipher_padding;

typedef enum {
    PAL_CIPHER_OP_NONE = -1,
    PAL_CIPHER_OP_ENCRYPT,
    PAL_CIPHER_OP_DECRYPT,
    PAL_CIPHER_OP_MAX,
} pal_cipher_operation;

typedef struct pal_cipher_ctx pal_cipher_ctx;

pal_cipher_ctx *pal_cipher_new(pal_cipher_type type);

void pal_cipher_free(pal_cipher_ctx *ctx);

bool pal_cipher_reset(pal_cipher_ctx *ctx);

size_t pal_cipher_get_block_size(pal_cipher_ctx *ctx);

size_t pal_cipher_get_key_len(pal_cipher_ctx *ctx);

size_t pal_cipher_get_iv_len(pal_cipher_ctx *ctx);

bool pal_cipher_set_padding(pal_cipher_ctx *ctx, pal_cipher_padding padding);

bool pal_cipher_begin(pal_cipher_ctx *ctx, pal_cipher_operation op, const uint8_t *key, const uint8_t *iv);

bool pal_cipher_update(pal_cipher_ctx *ctx, const void *in, size_t ilen, void *out, size_t *olen);

bool pal_cipher_finsh(pal_cipher_ctx *ctx, void *out, size_t *olen);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_CIPHER_H_
