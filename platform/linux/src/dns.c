// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pal/net/dns.h>
#include <pal/memory.h>
#include <HAPPlatform.h>

typedef struct pal_dns_resolve_ctx {
    pal_dns_response_cb cb;
    void *arg;
    struct addrinfo hint;
    struct addrinfo *result;
    int ret;
    pthread_t tid;
    char hostname[0];
} pal_dns_resolve_ctx;

static const HAPLogObject dns_log_obj = {
    .subsystem = kHAPPlatform_LogSubsystem,
    .category = "dns",
};

static const int pal_dns_af_mapping[] = {
    [PAL_ADDR_FAMILY_UNSPEC] = AF_UNSPEC,
    [PAL_ADDR_FAMILY_IPV4] = AF_INET,
    [PAL_ADDR_FAMILY_IPV6] = AF_INET6
};

static void pal_dns_destroy_resolve_ctx(void *arg) {
    struct pal_dns_resolve_ctx *ctx = arg;

    if (ctx->result) {
        freeaddrinfo(ctx->result);
        ctx->result = NULL;
    }
    pal_mem_free(ctx);
}

static void pal_dns_req_ctx_schedule(void* _Nullable context, size_t contextSize) {
    HAPPrecondition(context);
    struct pal_dns_resolve_ctx *ctx = *(struct pal_dns_resolve_ctx **)context;

    pthread_join(ctx->tid, NULL);

    pal_dns_response_cb cb = ctx->cb;
    void *arg = ctx->arg;
    const char *addr = NULL;
    const char *err = NULL;
    pal_addr_family af = PAL_ADDR_FAMILY_UNSPEC;

    if (ctx->ret) {
        err = gai_strerror(ctx->ret);
        goto done;
    }

    char buf[128];
    struct addrinfo *result = ctx->result;
    switch (result->ai_addr->sa_family) {
    case AF_INET: {
        struct sockaddr_in *in = (struct sockaddr_in *)result->ai_addr;
        addr = inet_ntop(AF_INET, &in->sin_addr, buf, sizeof(buf));
        af = PAL_ADDR_FAMILY_IPV4;
    } break;
    case AF_INET6: {
        struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)result->ai_addr;
        addr = inet_ntop(AF_INET6, &in6->sin6_addr, buf, sizeof(buf));
        af = PAL_ADDR_FAMILY_IPV6;
    } break;
    }

    if (!addr) {
        err = "invalid address";
    }

done:
    pal_dns_destroy_resolve_ctx(ctx);
    cb(err, addr, af, arg);
}

void pal_dns_init() {
}

void pal_dns_deinit() {
}

static void *pal_dns_resolve(void *arg) {
    struct pal_dns_resolve_ctx *ctx = arg;

    pthread_cleanup_push(pal_dns_destroy_resolve_ctx, ctx);

    ctx->ret = getaddrinfo(ctx->hostname, NULL, &ctx->hint, &ctx->result);
    HAPAssert(HAPPlatformRunLoopScheduleCallback(pal_dns_req_ctx_schedule,
        &ctx, sizeof(ctx)) == kHAPError_None);

    pthread_cleanup_pop(0);

    return NULL;
}

pal_dns_req_ctx *pal_dns_start_request(const char *hostname, pal_addr_family af,
    pal_dns_response_cb response_cb, void *arg) {
    HAPPrecondition(hostname);
    HAPPrecondition(af >= PAL_ADDR_FAMILY_UNSPEC && af <= PAL_ADDR_FAMILY_IPV6);
    HAPPrecondition(response_cb);

    size_t namelen = strlen(hostname);
    pal_dns_resolve_ctx *ctx = pal_mem_calloc(sizeof(*ctx) + namelen + 1);
    if (!ctx) {
        HAPLogError(&dns_log_obj, "%s: Failed to alloc memory.", __func__);
        return NULL;
    }
    memcpy(ctx->hostname, hostname, namelen);
    ctx->hostname[namelen] = '\0';
    ctx->cb = response_cb;
    ctx->arg = arg;
    ctx->hint.ai_family = pal_dns_af_mapping[af];
    ctx->hint.ai_flags = AI_ADDRCONFIG;
    ctx->result = NULL;

    int ret = pthread_create(&ctx->tid, NULL, pal_dns_resolve, ctx);
    if (ret) {
        HAPLogError(&dns_log_obj, "%s: Failed to create a request thread.", __func__);
        pal_mem_free(ctx);
        return NULL;
    }
    return (pal_dns_req_ctx *)ctx->tid;
}

void pal_dns_cancel_request(pal_dns_req_ctx *ctx) {
    pthread_t tid = (pthread_t)ctx;
    if (pthread_kill(tid, 0) == 0) {
        pthread_cancel(tid);
    }
    pthread_join(tid, NULL);
}
