// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_CRYPTO_MD_H_
#define PLATFORM_INCLUDE_PAL_CRYPTO_MD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pal/types.h>

/**
 * @brief     Supported message digests.
 *
 * @warning   MD4, MD5 and SHA-1 are considered weak message digests and
 *            their use constitutes a security risk. We recommend considering
 *            stronger message digests instead.
 */
typedef enum {
    PAL_MD_MD4,       /**< The MD4 message digest. */
    PAL_MD_MD5,       /**< The MD5 message digest. */
    PAL_MD_SHA1,      /**< The SHA-1 message digest. */
    PAL_MD_SHA224,    /**< The SHA-224 message digest. */
    PAL_MD_SHA256,    /**< The SHA-256 message digest. */
    PAL_MD_SHA384,    /**< The SHA-384 message digest. */
    PAL_MD_SHA512,    /**< The SHA-512 message digest. */
    PAL_MD_RIPEMD160, /**< The RIPEMD-160 message digest. */
    PAL_MD_TYPE_MAX,
} pal_md_type;

/**
 * Initializes a message-digest context.
 *
 * @param ctx The uninitialized message-digest context.
 * @param type Type of digest.
 * @param key If key is set, HMAC will be used.
 * @param len Length of @p key.
 *
 * @return true on success
 * @return false on failure.
 */
bool pal_md_ctx_init(pal_md_ctx *ctx, pal_md_type type, const void *key, size_t len);

/**
 * @brief Releases resources associated with a initialized message-digest context.
 *
 * @param ctx The initialized message-digest context.
 */
void pal_md_ctx_deinit(pal_md_ctx *ctx);

/**
 * Return the size of message-digest.
 */
size_t pal_md_get_size(pal_md_ctx *ctx);

/**
 * Update message-digest context with data.
 *
 * @param ctx message-digest context.
 * @param data The data to update.
 * @param len The length of the data.
 * @return true on success
 * @return false on failure.
 */
bool pal_md_update(pal_md_ctx *ctx, const void *data, size_t len);

/**
 * Return the digest of the data passed to the update() method so far.
 *
 * @param ctx message-digest context.
 * @param output The buffer to receive the hash value. Its size must be
 *  (at least) the size returned by pal_md_get_size().
 * @return true on success
 * @return false on failure.
 */
bool pal_md_digest(pal_md_ctx *ctx, uint8_t *output);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_CRYPTO_MD_H_
