// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <mbedtls/ssl.h>
#include <mbedtls/error.h>
#include <pal/ssl.h>
#include <pal/ssl_int.h>
#include <HAPPlatform.h>

#define MBEDTLS_PRINT_ERROR(func, err) \
do { \
    char buf[128]; \
    mbedtls_strerror(err, buf, sizeof(buf)); \
    HAPLogError(&ssl_log_obj, \
        "%s: %s() returned -%04X: %s", __func__, #func, -err, buf); \
} while (0)

typedef struct pal_ssl_ctx_int {
    uint16_t id;
    void *bio;
    pal_ssl_bio_method bio_method;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
} pal_ssl_ctx_int;
HAP_STATIC_ASSERT(sizeof(pal_ssl_ctx) >= sizeof(pal_ssl_ctx_int), pal_ssl_ctx_int);

static const HAPLogObject ssl_log_obj = {
    .subsystem = kHAPPlatform_LogSubsystem,
    .category = "ssl",
};

static const int ssl_endpoint_mapping[] = {
    [PAL_SSL_ENDPOINT_CLIENT] = MBEDTLS_SSL_IS_CLIENT,
    [PAL_SSL_ENDPOINT_SERVER] = MBEDTLS_SSL_IS_SERVER
};

static const int ssl_transport_mapping[] = {
    [PAL_SSL_TYPE_TLS] = MBEDTLS_SSL_TRANSPORT_STREAM,
    [PAL_SSL_TYPE_DTLS] = MBEDTLS_SSL_TRANSPORT_DATAGRAM
};

static uint16_t gssl_count;

static int pal_mbedtls_rng(void *arg, unsigned char *buf, size_t len) {
    HAPPlatformRandomNumberFill(buf, len);
    return 0;
}

static void pal_mbedtls_dbg_cb(void *arg, int level, const char *file, int line, const char *str) {
    pal_ssl_ctx_int *ctx = arg;
    HAPLogType type = kHAPLogType_Debug;
    switch (level) {
    case 1:
        type = kHAPLogType_Error;
        break;
    case 2:
        type = kHAPLogType_Default;
        break;
    case 3:
        type = kHAPLogType_Info;
        break;
    case 4:
        type = kHAPLogType_Debug;
        break;
    default:
        break;
    }
    HAPLogWithType(&ssl_log_obj, type, "(id=%u) %s", ctx->id, str);
}

static int pal_mbedtls_ssl_send(void *arg, const unsigned char *buf, size_t len) {
    pal_ssl_ctx_int *ctx = arg;

    pal_err err = ctx->bio_method.write(ctx->bio, buf, &len);
    switch (err) {
    case PAL_ERR_AGAIN:
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    case PAL_ERR_OK:
        return len;
    default:
        return -1;
    }
}

static int pal_mbedtls_ssl_recv(void *arg, unsigned char *buf, size_t len) {
    pal_ssl_ctx_int *ctx = arg;

    pal_err err = ctx->bio_method.read(ctx->bio, buf, &len);
    switch (err) {
    case PAL_ERR_AGAIN:
        return MBEDTLS_ERR_SSL_WANT_READ;
    case PAL_ERR_OK:
        return len;
    default:
        return -1;
    }
}

bool pal_ssl_ctx_init(pal_ssl_ctx *_ctx, pal_ssl_type type, pal_ssl_endpoint ep,
    const char *hostname, void *bio, const pal_ssl_bio_method *bio_method) {
    HAPPrecondition(_ctx);
    HAPPrecondition(type == PAL_SSL_TYPE_TLS || type == PAL_SSL_TYPE_DTLS);
    HAPPrecondition(ep == PAL_SSL_ENDPOINT_CLIENT || ep == PAL_SSL_ENDPOINT_SERVER);
    HAPPrecondition(bio);
    HAPPrecondition(bio_method);

    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    mbedtls_ssl_init(&ctx->ssl);
    mbedtls_ssl_config_init(&ctx->conf);

    int ret = mbedtls_ssl_config_defaults(&ctx->conf, ssl_endpoint_mapping[ep],
        ssl_transport_mapping[type], MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret) {
        MBEDTLS_PRINT_ERROR(mbedtls_ssl_config_defaults, ret);
        return false;
    }

    ctx->bio = bio;
    ctx->bio_method = *bio_method;
    mbedtls_ssl_set_bio(&ctx->ssl, ctx, pal_mbedtls_ssl_send, pal_mbedtls_ssl_recv, NULL);

    if (hostname) {
        mbedtls_ssl_set_hostname(&ctx->ssl, hostname);
    }

    pal_ssl_set_default_ca_chain(&ctx->conf);
    mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_rng(&ctx->conf, pal_mbedtls_rng, NULL);
    mbedtls_ssl_conf_dbg(&ctx->conf, pal_mbedtls_dbg_cb, ctx);

    ret = mbedtls_ssl_setup(&ctx->ssl, &ctx->conf);
    if (ret) {
        MBEDTLS_PRINT_ERROR(mbedtls_ssl_setup, ret);
        mbedtls_ssl_config_free(&ctx->conf);
        return false;
    }

    ctx->id = ++gssl_count;

    return true;
}

void pal_ssl_ctx_deinit(pal_ssl_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
}

pal_err pal_ssl_handshake(pal_ssl_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    int ret = mbedtls_ssl_handshake(&ctx->ssl);
    switch (ret) {
    case 0:
        return PAL_ERR_OK;
    case MBEDTLS_ERR_SSL_WANT_READ:
        return PAL_ERR_WANT_READ;
    case MBEDTLS_ERR_SSL_WANT_WRITE:
        return PAL_ERR_WANT_WRITE;
    default:
        MBEDTLS_PRINT_ERROR(mbedtls_ssl_handshake, ret);
        return PAL_ERR_UNKNOWN;
    }
}

pal_err pal_ssl_read(pal_ssl_ctx *_ctx, void *buf, size_t *len) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    int ret = mbedtls_ssl_read(&ctx->ssl, buf, *len);
    if (ret > 0) {
        *len = ret;
        return PAL_ERR_OK;
    } else if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
        return PAL_ERR_AGAIN;
    } else {
        MBEDTLS_PRINT_ERROR(mbedtls_ssl_read, ret);
        return PAL_ERR_UNKNOWN;
    }
}

pal_err pal_ssl_write(pal_ssl_ctx *_ctx, const void *data, size_t *len) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    int ret = mbedtls_ssl_write(&ctx->ssl, data, *len);
    if (ret >= 0) {
        *len = ret;
        return PAL_ERR_OK;
    } else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        return PAL_ERR_AGAIN;
    } else {
        MBEDTLS_PRINT_ERROR(mbedtls_ssl_write, ret);
        return PAL_ERR_UNKNOWN;
    }
}

bool pal_ssl_pending(pal_ssl_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    return mbedtls_ssl_check_pending(&ctx->ssl);
}
