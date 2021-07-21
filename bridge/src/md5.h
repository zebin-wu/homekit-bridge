// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef BRIDGE_SRC_MD5_H_
#define BRIDGE_SRC_MD5_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define MD5_HASHSIZE    16

/**
 * MD5 context.
 */
typedef struct {
    uint32_t digest[MD5_HASHSIZE / sizeof(uint32_t)];
    size_t len;
} md5_ctx;

/**
 * Initialize a MD5 context.
 *
 * @param ctx MD5 context.
 */
void md5_init(md5_ctx *ctx);

/**
 * Update MD5 context with data.
 *
 * @param ctx MD5 context.
 * @param data The data to update.
 * @param len The length of the data.
 *
 * @return true if update completed, means len is not the multiples of 64.
*          false if you can continue update.
 */
bool md5_update(md5_ctx *ctx, const void *data, size_t len);

/**
 * Return the digest of the data passed to the update() method so far.
 *
 * @param ctx MD5 context.
 * @param output The buffer to receive the hash value. Its size must be
 *  (at least) MD5_HASHSIZE.
 */
void md5_digest(md5_ctx *ctx, uint8_t output[MD5_HASHSIZE]);

#ifdef __cplusplus
}
#endif

#endif  // BRIDGE_SRC_MD5_H_
