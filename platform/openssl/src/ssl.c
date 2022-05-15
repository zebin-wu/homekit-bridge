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
    BIO *in_bio;
    BIO *out_bio;
} pal_ssl_ctx_int;
HAP_STATIC_ASSERT(sizeof(pal_ssl_ctx) >= sizeof(pal_ssl_ctx_int), pal_ssl_ctx_int);

static const HAPLogObject ssl_log_obj = {
    .subsystem = kHAPPlatform_LogSubsystem,
    .category = "ssl",
};

void pal_ssl_init() {
}

void pal_ssl_deinit() {
}

bool pal_ssl_ctx_init(pal_ssl_ctx *_ctx, pal_ssl_type type, pal_ssl_endpoint ep, const char *hostname) {
    HAPPrecondition(_ctx);
    HAPPrecondition(type == PAL_SSL_TYPE_TLS || type == PAL_SSL_TYPE_DTLS);
    HAPPrecondition(ep == PAL_SSL_ENDPOINT_CLIENT || ep == PAL_SSL_ENDPOINT_SERVER);

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
        goto err;
    }

    ctx->in_bio = BIO_new(BIO_s_mem());
    if (!ctx->in_bio) {
        LOG_OPENSSL_ERROR("Failed to new in BIO");
        goto err1;
    }
    BIO_set_mem_eof_return(ctx->in_bio, -1);

    ctx->out_bio = BIO_new(BIO_s_mem());
    if (!ctx->out_bio) {
        LOG_OPENSSL_ERROR("Failed to new out BIO");
        goto err2;
    }
    BIO_set_mem_eof_return(ctx->out_bio, -1);

    SSL_set_bio(ctx->ssl, ctx->in_bio, ctx->out_bio);

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

    return true;

err2:
    BIO_free(ctx->in_bio);
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

bool pal_ssl_finshed(pal_ssl_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    return SSL_is_init_finished(ctx->ssl);
}

static inline size_t pal_ssl_bio_pending(pal_ssl_ctx_int *ctx) {
    return BIO_ctrl_pending(ctx->out_bio);
}

static bool pal_ssl_bio_read(pal_ssl_ctx_int *ctx, void *data, size_t *len) {
    size_t pending = pal_ssl_bio_pending(ctx);

    if (!pending) {
        *len = 0;
        return true;
    }

    size_t buflen = *len;
    size_t readbytes;
    while (pending && *len > 0) {
        if (!BIO_read_ex(ctx->out_bio, data, *len, &readbytes)) {
            *len = 0;
            LOG_OPENSSL_ERROR("Failed to read BIO");
            return false;
        }
        data += readbytes;
        *len -= readbytes;
        pending = pal_ssl_bio_pending(ctx);
    }
    *len = buflen - *len;
    return true;
}

static bool pal_ssl_bio_write(pal_ssl_ctx_int *ctx, const void *data, size_t len) {
    size_t written;
    while (len) {
        if (!BIO_write_ex(ctx->in_bio, data, len, &written)) {
            LOG_OPENSSL_ERROR("Failed to write BIO");
            return false;
        }
        data += written;
        len -= written;
    }

    return true;
}

pal_err pal_ssl_handshake(pal_ssl_ctx *_ctx, const void *in, size_t ilen, void *out, size_t *olen) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;
    HAPPrecondition((in && ilen > 0) || (!in && ilen == 0));
    HAPPrecondition(out);
    HAPPrecondition(olen);
    HAPPrecondition(*olen > 0);

    if (pal_ssl_finshed(_ctx)) {
        HAPLogError(&ssl_log_obj, "%s: Handshake is already finshed.", __func__);
        *olen = 0;
        return PAL_ERR_INVALID_STATE;
    }

    if (ilen && in) {
        if (!pal_ssl_bio_write(ctx, in, ilen)) {
            *olen = 0;
            return PAL_ERR_UNKNOWN;
        }
    }

    if (pal_ssl_finshed(_ctx)) {
        *olen = 0;
        return PAL_ERR_OK;
    }
    int ret = SSL_do_handshake(ctx->ssl);
    if (ret == 1) {
        *olen = 0;
        return PAL_ERR_OK;
    } else if (ret < 0) {
        int err = SSL_get_error(ctx->ssl, ret);
        ERR_clear_error();
        switch (err) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            if (pal_ssl_bio_read(ctx, out, olen)) {
                if (pal_ssl_bio_pending(ctx)) {
                    return PAL_ERR_AGAIN;
                } else {
                    return PAL_ERR_OK;
                }
            }
            break;
        default:
            HAPLogError(&ssl_log_obj, "%s: Failed to do handshake: %d",
                __func__, err);
            break;
        }
    }
    return PAL_ERR_UNKNOWN;
}

pal_err pal_ssl_encrypt(pal_ssl_ctx *_ctx, const void *in, size_t ilen, void *out, size_t *olen) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;
    HAPPrecondition((in && ilen > 0) || (!in && ilen == 0));
    HAPPrecondition(out);
    HAPPrecondition(olen);
    HAPPrecondition(*olen > 0);

    while (ilen) {
        int written = SSL_write(ctx->ssl, in, ilen);
        if (written <= 0) {
            int err = SSL_get_error(ctx->ssl, written);
            ERR_clear_error();
            HAPLogError(&ssl_log_obj, "%s: Failed to write SSL: %d", __func__, err);
            return PAL_ERR_UNKNOWN;
        }
        ilen -= written;
        in += written;
    }
    if (!pal_ssl_bio_read(ctx, out, olen)) {
        return PAL_ERR_UNKNOWN;
    }
    if (pal_ssl_bio_pending(ctx)) {
        return PAL_ERR_AGAIN;
    }

    return PAL_ERR_OK;
}

pal_err pal_ssl_decrypt(pal_ssl_ctx *_ctx, const void *in, size_t ilen, void *out, size_t *olen) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;
    HAPPrecondition((in && ilen > 0) || (!in && ilen == 0));
    HAPPrecondition(out);
    HAPPrecondition(olen);
    HAPPrecondition(*olen > 0);

    if (ilen && in) {
        if (!pal_ssl_bio_write(ctx, in, ilen)) {
            *olen = 0;
            return PAL_ERR_UNKNOWN;
        }
    }

    size_t buflen = *olen;
    int readbytes;
    while (*olen > 0) {
        readbytes = SSL_read(ctx->ssl, out, *olen);
        if (readbytes <= 0) {
            int err = SSL_get_error(ctx->ssl, readbytes);
            ERR_clear_error();
            switch (err) {
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_ZERO_RETURN:
                *olen = buflen - *olen;
                return PAL_ERR_OK;
                break;
            default:
                HAPLogError(&ssl_log_obj, "%s: Failed to read SSL: %d", __func__, err);
                *olen = 0;
                return PAL_ERR_UNKNOWN;
            }
        }
        out += readbytes;
        *olen -= readbytes;
    }
    *olen = buflen - *olen;
    return SSL_pending(ctx->ssl) ? PAL_ERR_AGAIN : PAL_ERR_OK;
}
