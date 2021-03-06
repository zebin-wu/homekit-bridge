// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <mbedtls/ssl.h>
#include <mbedtls/error.h>
#include <sys/queue.h>
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

struct pal_ssl_buf {
    STAILQ_ENTRY(pal_ssl_buf) entry;
    pal_ssl_bio bio;
    char buf[0];
};

typedef struct pal_ssl_ctx_int {
    bool finshed;
    uint16_t id;
    pal_ssl_bio in_bio;
    pal_ssl_bio out_bio;
    STAILQ_HEAD(, pal_ssl_buf) outbuf_list_head;
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
    pal_ssl_ctx_int *ctx = arg;
    struct pal_ssl_buf *outbuf = STAILQ_FIRST(&ctx->outbuf_list_head);
    pal_ssl_bio *bio = NULL;
    if (outbuf) {
        bio = &outbuf->bio;
    } else {
        bio = &ctx->out_bio;
        if (bio->len == 0) {
            return MBEDTLS_ERR_SSL_WANT_READ;
        }
    }
    size_t readbytes = HAPMin(len, bio->len);
    memcpy(buf, bio->data, readbytes);
    bio->data += readbytes;
    bio->len -= readbytes;

    if (outbuf && bio->len == 0) {
        STAILQ_REMOVE_HEAD(&ctx->outbuf_list_head, entry);
        pal_mem_free(outbuf);
    }
    return readbytes;
}

bool pal_ssl_ctx_init(pal_ssl_ctx *_ctx, pal_ssl_type type, pal_ssl_endpoint ep, const char *hostname) {
    HAPPrecondition(_ctx);
    HAPPrecondition(type == PAL_SSL_TYPE_TLS || type == PAL_SSL_TYPE_DTLS);
    HAPPrecondition(ep == PAL_SSL_ENDPOINT_CLIENT || ep == PAL_SSL_ENDPOINT_SERVER);

    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    mbedtls_ssl_init(&ctx->ssl);
    mbedtls_ssl_config_init(&ctx->conf);

    int ret = mbedtls_ssl_config_defaults(&ctx->conf, ssl_endpoint_mapping[ep],
        ssl_transport_mapping[type], MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret) {
        MBEDTLS_PRINT_ERROR(mbedtls_ssl_config_defaults, ret);
        return false;
    }

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

    ctx->finshed = false;
    ctx->id = ++gssl_count;
    STAILQ_INIT(&ctx->outbuf_list_head);

    return true;
}

void pal_ssl_ctx_deinit(pal_ssl_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    for (struct pal_ssl_buf *t = STAILQ_FIRST(&ctx->outbuf_list_head); t;) {
        struct pal_ssl_buf *cur = t;
        t = STAILQ_NEXT(t, entry);
        pal_mem_free(cur);
    }
    mbedtls_ssl_free(&ctx->ssl);
    mbedtls_ssl_config_free(&ctx->conf);
}

bool pal_ssl_finshed(pal_ssl_ctx *_ctx) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;

    return ctx->finshed;
}

pal_err pal_ssl_handshake(pal_ssl_ctx *_ctx, const void *in, size_t ilen, void *out, size_t *olen) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;
    HAPPrecondition((in && ilen > 0) || (!in && ilen == 0));
    HAPPrecondition(out);
    HAPPrecondition(olen);
    HAPPrecondition(*olen > 0);

    if (ctx->finshed) {
        HAPLogError(&ssl_log_obj, "%s: Handshake is already finshed.", __func__);
        *olen = 0;
        return PAL_ERR_INVALID_STATE;
    }

    pal_ssl_bio_init(&ctx->out_bio, (void *)in, ilen);
    pal_ssl_bio_init(&ctx->in_bio, out, *olen);

    pal_err err = PAL_ERR_UNKNOWN;
    int ret = mbedtls_ssl_handshake(&ctx->ssl);
    switch (ret) {
    case 0:
        ctx->finshed = true;
    case MBEDTLS_ERR_SSL_WANT_READ:
        HAPAssert(ctx->out_bio.len == 0);
        err = PAL_ERR_OK;
        *olen = *olen - ctx->in_bio.len;
        break;
    case MBEDTLS_ERR_SSL_WANT_WRITE:
        HAPAssert(ctx->out_bio.len == 0);
        err = PAL_ERR_AGAIN;
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

pal_err pal_ssl_encrypt(pal_ssl_ctx *_ctx, const void *in, size_t ilen, void *out, size_t *olen) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;
    HAPPrecondition((in && ilen > 0) || (!in && ilen == 0));
    HAPPrecondition(out);
    HAPPrecondition(olen);
    HAPPrecondition(*olen > 0);

    pal_ssl_bio_init(&ctx->in_bio, out, *olen);

    pal_err err = PAL_ERR_UNKNOWN;
    int ret = mbedtls_ssl_write(&ctx->ssl, in, ilen);
    if (ret == ilen) {
        err = PAL_ERR_OK;
        *olen = *olen - ctx->in_bio.len;
    } else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        err = PAL_ERR_AGAIN;
        *olen = *olen - ctx->in_bio.len;
    } else {
        MBEDTLS_PRINT_ERROR(mbedtls_ssl_write, ret);
        *olen = 0;
    }

    pal_ssl_bio_reset(&ctx->in_bio);
    return err;
}

pal_err pal_ssl_decrypt(pal_ssl_ctx *_ctx, const void *in, size_t ilen, void *out, size_t *olen) {
    HAPPrecondition(_ctx);
    pal_ssl_ctx_int *ctx = (pal_ssl_ctx_int *)_ctx;
    HAPPrecondition((in && ilen > 0) || (!in && ilen == 0));
    HAPPrecondition(out);
    HAPPrecondition(olen);
    HAPPrecondition(*olen > 0);

    pal_ssl_bio_init(&ctx->out_bio, (void *)in, ilen);

    size_t len = *olen;
    while (len > 0) {
        int ret = mbedtls_ssl_read(&ctx->ssl, out, len);
        if (ret > 0) {
            out += ret;
            len -= ret;
        } else if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
            *olen = *olen - len;
            pal_ssl_bio_reset(&ctx->out_bio);
            return PAL_ERR_OK;
        } else {
            MBEDTLS_PRINT_ERROR(mbedtls_ssl_read, ret);
            *olen = 0;
            pal_ssl_bio_reset(&ctx->out_bio);
            return PAL_ERR_UNKNOWN;
        }
    }

    if (ctx->out_bio.len) {
        size_t len = ctx->out_bio.len;
        struct pal_ssl_buf *buf = pal_mem_alloc(sizeof(*buf) + len);
        if (!buf) {
            HAPLogError(&ssl_log_obj, "%s: Failed to alloc SSL context.", __func__);
            return PAL_ERR_ALLOC;
        }
        STAILQ_INSERT_TAIL(&ctx->outbuf_list_head, buf, entry);
        buf->bio.len = len;
        memcpy(buf->buf, ctx->out_bio.data, len);
        buf->bio.data = buf->buf;
    }
    pal_ssl_bio_reset(&ctx->out_bio);
    return mbedtls_ssl_check_pending(&ctx->ssl) ? PAL_ERR_AGAIN : PAL_ERR_OK;
}
