// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_MD5_H_
#define PLATFORM_INCLUDE_PAL_MD5_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PAL_MD5_HASHSIZE    16

/**
 * MD5 context.
 */
typedef struct pal_md5_ctx pal_md5_ctx;

/**
 * New a MD5 context.
 *
 * @return MD5 context.
 */
pal_md5_ctx *pal_md5_new(void);

/**
 * Free a MD5 context.
 * 
 * @param ctx MD5 context.
 */
void pal_md5_free(pal_md5_ctx *ctx);

/**
 * Update MD5 context with data.
 *
 * @param ctx MD5 context.
 * @param data The data to update.
 * @param len The length of the data.
 */
void pal_md5_update(pal_md5_ctx *ctx, const void *data, size_t len);

/**
 * Return the digest of the data passed to the update() method so far.
 *
 * @param ctx MD5 context.
 * @param output The buffer to receive the hash value. Its size must be
 *  (at least) PAL_MD5_HASHSIZE.
 */
void pal_md5_digest(pal_md5_ctx *ctx, uint8_t output[PAL_MD5_HASHSIZE]);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_MD5_H_
