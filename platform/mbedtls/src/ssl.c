// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <mbedtls/ssl.h>
#include <mbedtls/error.h>
#include <pal/memory.h>
#include <pal/crypto/ssl.h>
#include <pal/crypto/ssl_int.h>
#include <HAPPlatform.h>

#define MBEDTLS_PRINT_ERROR(func, err) \
do { \
    char buf[128]; \
    mbedtls_strerror(err, buf, sizeof(buf)); \
    HAPLogError(&ssl_log_obj, \
        "%s: %s() returned -%04X: %s", __func__, #func, -err, buf); \
} while (0)

typedef struct {
    void *data;
    size_t len;
} pal_ssl_bio;

struct pal_ssl_ctx {
    bool finshed;
    uint16_t id;
    pal_ssl_bio in_bio;
    pal_ssl_bio out_bio;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
};

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

static inline void pal_ssl_bio_init(pal_ssl_bio *bio, void *data, size_t len) {
    bio->data = data;
    bio->len = len;
}

static inline void pal_ssl_bio_reset(pal_ssl_bio *bio) {
    bio->data = NULL;
    bio->len = 0;
}

static int pal_mbedtls_rng(void *arg, unsigned char *buf, size_t len) {
    HAPPlatformRandomNumberFill(buf, len);
    return 0;
}

static void pal_mbedtls_dbg_cb(void *arg, int level, const char *file, int line, const char *str) {
    pal_ssl_ctx *ctx = arg;
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
    pal_ssl_ctx *ctx = arg;
    pal_ssl_bio *bio = &ctx->in_bio;

    if (bio->len == 0) {
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    }
    size_t written = HAPMin(len, bio->len);
    memcpy(bio->data, buf, written);
    bio->data += written;
    bio->len -= written;
    return written;
}

static int pal_mbedtls_ssl_recv(void *arg, unsigned char *buf, size_t len) {
    pal_ssl_ctx *ctx = arg;
    pal_ssl_bio *bio = &ctx->out_bio;

    if (bio->len == 0) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }
    size_t readbytes = HAPMin(len, bio->len);
    memcpy(buf, bio->data, readbytes);
    bio->data += readbytes;
    bio->len -= readbytes;
    return readbytes;
}

pal_ssl_ctx *pal_ssl_create(pal_ssl_type type, pal_ssl_endpoint ep, const char *hostname) {
    HAPPrecondition(type == PAL_SSL_TYPE_TLS || type == PAL_SSL_TYPE_DTLS);
    HAPPrecondition(ep == PAL_SSL_ENDPOINT_CLIENT || ep == PAL_SSL_ENDPOINT_SERVER);

    pal_ssl_ctx *ctx = pal_mem_alloc(sizeof(*ctx));
    if (!ctx) {
        HAPLogError(&ssl_log_obj, "%s: Failed to alloc memory.", __func__);
        return NULL;
    }

    mbedtls_ssl_init(&ctx->ssl);
    mbedtls_ssl_config_init(&ctx->conf);

    int ret = mbedtls_ssl_config_defaults(&ctx->conf, ssl_endpoint_mapping[ep],
        ssl_transport_mapping[type], MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret) {
        MBEDTLS_PRINT_ERROR(mbedtls_ssl_config_defaults, ret);
        goto err;
    }

    ctx->finshed = false;
    ctx->id = ++gssl_count;

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
        goto err;
    }

    return ctx;

err:
    pal_mem_free(ctx);
    return NULL;
}

void pal_ssl_destroy(pal_ssl_ctx *ctx) {
    if (!ctx) {
        return;
    }
    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
    pal_mem_free(ctx);
}

bool pal_ssl_finshed(pal_ssl_ctx *ctx) {
    HAPPrecondition(ctx);
    return ctx->finshed;
}

pal_ssl_err pal_ssl_handshake(pal_ssl_ctx *ctx, const void *in, size_t ilen, void *out, size_t *olen) {
    HAPPrecondition(ctx);
    HAPPrecondition((in && ilen > 0) || (!in && ilen == 0));
    HAPPrecondition(out);
    HAPPrecondition(olen);
    HAPPrecondition(*olen > 0);

    if (ctx->finshed) {
        HAPLogError(&ssl_log_obj, "%s: Handshake is already finshed.", __func__);
        *olen = 0;
        return PAL_SSL_ERR_INVALID_STATE;
    }

    pal_ssl_bio_init(&ctx->out_bio, (void *)in, ilen);
    pal_ssl_bio_init(&ctx->in_bio, out, *olen);

    pal_ssl_err err = PAL_SSL_ERR_UNKNOWN;
    int ret = mbedtls_ssl_handshake(&ctx->ssl);
    switch (ret) {
    case 0:
        ctx->finshed = true;
    case MBEDTLS_ERR_SSL_WANT_READ:
        HAPAssert(ctx->out_bio.len == 0);
        err = PAL_SSL_ERR_OK;
        *olen = *olen - ctx->in_bio.len;
        break;
    case MBEDTLS_ERR_SSL_WANT_WRITE:
        HAPAssert(ctx->out_bio.len == 0);
        err = PAL_SSL_ERR_AGAIN;
        *olen = *olen - ctx->in_bio.len;
        break;
    default:
        MBEDTLS_PRINT_ERROR(mbedtls_ssl_handshake, ret);
        *olen = 0;
    }

    pal_ssl_bio_reset(&ctx->in_bio);
    pal_ssl_bio_reset(&ctx->out_bio);
    return err;
}

pal_ssl_err pal_ssl_encrypt(pal_ssl_ctx *ctx, const void *in, size_t ilen, void *out, size_t *olen) {
    HAPPrecondition(ctx);
    HAPPrecondition((in && ilen > 0) || (!in && ilen == 0));
    HAPPrecondition(out);
    HAPPrecondition(olen);
    HAPPrecondition(*olen > 0);

    pal_ssl_bio_init(&ctx->in_bio, out, *olen);

    pal_ssl_err err = PAL_SSL_ERR_UNKNOWN;
    int ret = mbedtls_ssl_write(&ctx->ssl, in, ilen);
    if (ret == ilen) {
        err = PAL_SSL_ERR_OK;
        *olen = *olen - ctx->in_bio.len;
    } else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        err = PAL_SSL_ERR_AGAIN;
        *olen = *olen - ctx->in_bio.len;
    } else {
        MBEDTLS_PRINT_ERROR(mbedtls_ssl_write, ret);
        *olen = 0;
    }

    pal_ssl_bio_reset(&ctx->in_bio);
    return err;
}

pal_ssl_err pal_ssl_decrypt(pal_ssl_ctx *ctx, const void *in, size_t ilen, void *out, size_t *olen) {
    HAPPrecondition(ctx);
    HAPPrecondition((in && ilen > 0) || (!in && ilen == 0));
    HAPPrecondition(out);
    HAPPrecondition(olen);
    HAPPrecondition(*olen > 0);

    pal_ssl_bio_init(&ctx->out_bio, (void *)in, ilen);

    pal_ssl_err err = PAL_SSL_ERR_UNKNOWN;
    int ret = mbedtls_ssl_read(&ctx->ssl, out, *olen);
    if (ret > 0) {
        HAPAssert(ctx->out_bio.len == 0);
        err = mbedtls_ssl_check_pending(&ctx->ssl) ? PAL_SSL_ERR_AGAIN : PAL_SSL_ERR_OK;
        *olen = ret;
    } else {
        MBEDTLS_PRINT_ERROR(mbedtls_ssl_read, ret);
        *olen = 0;
    }

    pal_ssl_bio_reset(&ctx->out_bio);
    return err;
}
