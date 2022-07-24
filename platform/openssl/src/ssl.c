// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <pal/memory.h>
#include <pal/crypto/ssl.h>
#include <HAPPlatform.h>

#define LOG_OPENSSL_ERROR(msg) \
do { \
    char buf[128]; \
    ERR_error_string_n(ERR_get_error(), buf, sizeof(buf)); \
    HAPLogError(&ssl_log_obj, \
        "%s: %s: %s", __func__, msg, buf); \
} while (0)

typedef struct pal_ssl_ctx_int {
    SSL_CTX *ctx;
    SSL *ssl;
    BIO *bio;
    void *bio_ctx;
    pal_ssl_bio_method bio_method;
} pal_ssl_ctx_int;
HAP_STATIC_ASSERT(sizeof(pal_ssl_ctx) >= sizeof(pal_ssl_ctx_int), pal_ssl_ctx_int);

static const HAPLogObject ssl_log_obj = {
    .subsystem = kHAPPlatform_LogSubsystem,
    .category = "ssl",
};

static BIO_METHOD *gbio_method;

static int pal_ssl_bio_read_ex(BIO *bio, char *buf, size_t len, size_t *readbytes) {
    if (!buf) {
        return 0;
    }
    pal_ssl_ctx_int *ctx = BIO_get_ex_data(bio, 0);
    *readbytes = len;
    pal_err err = ctx->bio_method.read(ctx->bio_ctx, buf, readbytes);
    switch (err) {
    case PAL_ERR_AGAIN:
        *readbytes = 0;
        BIO_set_retry_read(bio);
        return 0;
    case PAL_ERR_OK:
        if (*readbytes == 0) {
            BIO_set_flags(bio, BIO_FLAGS_IN_EOF);
            return 0;
        }
        return 1;
    default:
        *readbytes = 0;
        return 0;
    }
}

static int pal_ssl_bio_write_ex(BIO *bio, const char *data, size_t len, size_t *written) {
    pal_ssl_ctx_int *ctx = BIO_get_ex_data(bio, 0);
    *written = len;
    pal_err err = ctx->bio_method.write(ctx->bio_ctx, data, written);
    switch (err) {
    case PAL_ERR_AGAIN:
        *written = 0;
        BIO_set_retry_write(bio);
        return 0;
    case PAL_ERR_OK:
        return 1;
    default:
        *written = 0;
        return 0;
    }
}

static int pal_ssl_bio_read(BIO *bio, char *buf, int len) {
    size_t readbytes;
    if (pal_ssl_bio_read_ex(bio, buf, len, &readbytes)) {
        return readbytes;
    }
    return -1;
}

static int pal_ssl_bio_write(BIO *bio, const char *data, int len) {
    size_t written;
    if (pal_ssl_bio_write_ex(bio, data, len, &written)) {
        return written;
    }
    return -1;
}

static long pal_ssl_bio_ctrl(BIO *bio, int cmd, long larg, void *parg) {
    int ret = 0;
    switch (cmd) {
    case BIO_CTRL_FLUSH:
        ret = 1;
    default:
        break;
    }
    return ret;
}

void pal_ssl_init() {
    gbio_method = BIO_meth_new(BIO_TYPE_SOCKET, "socket");
    HAPAssert(gbio_method);
    BIO_meth_set_read_ex(gbio_method, pal_ssl_bio_read_ex);
    BIO_meth_set_write_ex(gbio_method, pal_ssl_bio_write_ex);
    BIO_meth_set_read(gbio_method, pal_ssl_bio_read);
    BIO_meth_set_write(gbio_method, pal_ssl_bio_write);
    BIO_meth_set_ctrl(gbio_method, pal_ssl_bio_ctrl);
}

void pal_ssl_deinit() {
    BIO_meth_free(gbio_method);
    gbio_method = NULL;
}

bool pal_ssl_ctx_init(pal_ssl_ctx *_ctx, pal_ssl_type type, pal_ssl_endpoint ep,
    const char *hostname, void *bio, const pal_ssl_bio_method *bio_method) {
    HAPPrecondition(_ctx);
    HAPPrecondition(type == PAL_SSL_TYPE_TLS || type == PAL_SSL_TYPE_DTLS);
    HAPPrecondition(ep == PAL_SSL_ENDPOINT_CLIENT || ep == PAL_SSL_ENDPOINT_SERVER);
    HAPPrecondition(bio);
    HAPPrecondition(bio_method);

    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    const SSL_METHOD *method;
    switch (ep) {
    case PAL_SSL_ENDPOINT_CLIENT:
        switch (type) {
        case PAL_SSL_TYPE_TLS:
            method = TLS_client_method();
            break;
        case PAL_SSL_TYPE_DTLS:
            method = DTLS_client_method();
            break;
        }
        break;
    case PAL_SSL_ENDPOINT_SERVER:
        switch (type) {
        case PAL_SSL_TYPE_TLS:
            method = TLS_server_method();
            break;
        case PAL_SSL_TYPE_DTLS:
            method = DTLS_server_method();
            break;
        }
        break;
    }

    ctx->ctx = SSL_CTX_new(method);
    if (!ctx->ctx) {
        LOG_OPENSSL_ERROR("Failed to new SSL context");
        return false;
    }

    ctx->ssl = SSL_new(ctx->ctx);
    if (!ctx->ssl) {
        LOG_OPENSSL_ERROR("Failed to new SSL connection");
        ERR_clear_error();
        goto err;
    }

    ctx->bio = BIO_new(gbio_method);
    if (!ctx->bio) {
        LOG_OPENSSL_ERROR("Failed to new a BIO");
        ERR_clear_error();
        goto err1;
    }
    if (!BIO_set_ex_data(ctx->bio, 0, ctx)) {
        LOG_OPENSSL_ERROR("Failed to set SSL context as BIO extra data");
        ERR_clear_error();
        goto err2;
    }

    SSL_set_bio(ctx->ssl, ctx->bio, ctx->bio);

    if (hostname) {
        SSL_set_tlsext_host_name(ctx->ssl, hostname);
    }

    switch (ep) {
    case PAL_SSL_ENDPOINT_CLIENT:
        SSL_set_connect_state(ctx->ssl);
        break;
    case PAL_SSL_ENDPOINT_SERVER:
        SSL_set_accept_state(ctx->ssl);
        break;
    }

    ctx->bio_ctx = bio;
    ctx->bio_method = *bio_method;

    return true;

err2:
    BIO_free(ctx->bio);
err1:
    SSL_free(ctx->ssl);
err:
    SSL_CTX_free(ctx->ctx);
    return false;
}

void pal_ssl_ctx_deinit(pal_ssl_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    SSL_free(ctx->ssl);
    SSL_CTX_free(ctx->ctx);
}

pal_err pal_ssl_handshake(pal_ssl_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    int ret = SSL_do_handshake(ctx->ssl);
    if (ret == 1) {
        return PAL_ERR_OK;
    } else {
        int err = SSL_get_error(ctx->ssl, ret);
        ERR_clear_error();
        switch (err) {
        case SSL_ERROR_WANT_READ:
            return PAL_ERR_WANT_READ;
        case SSL_ERROR_WANT_WRITE:
            return PAL_ERR_WANT_WRITE;
        default:
            HAPLogError(&ssl_log_obj, "%s: Failed to do handshake: %d", __func__, err);
            return PAL_ERR_UNKNOWN;
        }
    }
}

pal_err pal_ssl_read(pal_ssl_ctx *_ctx, void *buf, size_t *len) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    int ret = SSL_read_ex(ctx->ssl, buf, *len, len);
    if (ret == 1) {
        return PAL_ERR_OK;
    } else {
        int err = SSL_get_error(ctx->ssl, ret);
        ERR_clear_error();
        switch (err) {
        case SSL_ERROR_WANT_READ:
            return PAL_ERR_AGAIN;
        default:
            HAPLogError(&ssl_log_obj, "%s: Failed to read SSL: %d", __func__, err);
            return PAL_ERR_UNKNOWN;
        }
    }
}

pal_err pal_ssl_write(pal_ssl_ctx *_ctx, const void *data, size_t *len) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    int ret = SSL_write_ex(ctx->ssl, data, *len, len);
    if (ret == 1) {
        return PAL_ERR_OK;
    } else {
        int err = SSL_get_error(ctx->ssl, ret);
        ERR_clear_error();
        switch (err) {
        case SSL_ERROR_WANT_WRITE:
            return PAL_ERR_AGAIN;
        default:
            HAPLogError(&ssl_log_obj, "%s: Failed to write SSL: %d", __func__, err);
            return PAL_ERR_UNKNOWN;
        }
    }
}

bool pal_ssl_pending(pal_ssl_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    return SSL_pending(ctx->ssl);
}
