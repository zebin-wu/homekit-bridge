// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_SSL_H_
#define PLATFORM_INCLUDE_PAL_SSL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pal/err.h>
#include <pal/types.h>

/**
 * SSL type.
 */
typedef enum pal_ssl_type {
    PAL_SSL_TYPE_TLS,
    PAL_SSL_TYPE_DTLS,
} pal_ssl_type;

/**
 * SSL endpoint.
 */
typedef enum pal_ssl_endpoint {
    PAL_SSL_ENDPOINT_CLIENT,
    PAL_SSL_ENDPOINT_SERVER,
} pal_ssl_endpoint;

/**
 * SSL basic I/O method.
 */
typedef struct {
    /**
     * Read underlying transport data.
     *
     * @param bio BIO context.
     * @param[out] buf The buf to hold data.
     * @param[inout] len The length of @p buf, to be updated with the actual number of Bytes read.
     *
     * @return PAL_ERR_OK on success.
     * @return PAL_ERR_AGAIN means you need to call this function again.
     * @return other error number on failure.
     */
    pal_err (*read)(void *bio, void *buf, size_t *len);

    /**
     * Write underlying transport data.
     *
     * @param ctx BIO context.
     * @param data A pointer to the data to be write.
     * @param[inout] len Length of @p data, to be updated with the actual number of Bytes write.
     *
     * @return PAL_ERR_OK on success.
     * @return PAL_ERR_AGAIN means you need to call this function again.
     * @return other error number on failure.
     */
    pal_err (*write)(void *bio, const void *data, size_t *len);
} pal_ssl_bio_method;

/**
 * Initializes a SSL context.
 *
 * @param ctx The SSL context to initialize.
 * @param type SSL type.
 * @param endpoint SSL endpoint.
 * @param hostname Server host name, only valid when the SSL endpoint is PAL_SSL_ENDPOINT_CLIENT.
 * @param bio BIO context.
 * @param bio_method BIO method.
 *
 * @return true on success
 * @return false on failure.
 */
bool pal_ssl_ctx_init(pal_ssl_ctx *ctx, pal_ssl_type type, pal_ssl_endpoint ep,
    const char *hostname, void *bio, const pal_ssl_bio_method *bio_method);

/**
 * Releases resources associated with a initialized SSL context.
 *
 * @param ctx The initialized SSL context.
 */
void pal_ssl_ctx_deinit(pal_ssl_ctx *ctx);

/**
 * Perform the SSL handshake.
 *
 * @param ctx SSL context.
 *
 * @return PAL_ERR_OK on success.
 * @return PAL_ERR_WANT_READ means you need to call this function
 *         again when the bio is readable.
 * @return PAL_ERR_WANT_WRITE means you need to call this function
 *         again when the bio is writable.
 * @return Other error numbers on failure.
 */
pal_err pal_ssl_handshake(pal_ssl_ctx *ctx);

/**
 * Read at most 'len' application data bytes
 *
 * @param ctx SSL context.
 * @param[out] buf The buf to hold data.
 * @param[inout] len The length of @p buf, to be updated with the actual number of Bytes read.
 *
 * @return PAL_ERR_OK on success.
 * @return PAL_ERR_AGAIN means you need to call this function again.
 * @return other error number on failure.
 */
pal_err pal_ssl_read(pal_ssl_ctx *ctx, void *buf, size_t *len);

/**
 * Try to write exactly 'len' application data bytes.
 *
 * @param ctx SSL context.
 * @param data A pointer to the data to be write.
 * @param[inout] len Length of @p data, to be updated with the actual number of Bytes write.
 *
 * @return PAL_ERR_OK on success.
 * @return PAL_ERR_AGAIN means you need to call this function again.
 * @return other error number on failure.
 */
pal_err pal_ssl_write(pal_ssl_ctx *ctx, const void *data, size_t *len);

/**
 * Check for readable bytes buffered in an SSL object.
 * 
 * @param ctx SSL context.
 * @return true means it has bytes pending.
 * @return false means nothing's pending.
 */
bool pal_ssl_pending(pal_ssl_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_SSL_H_
