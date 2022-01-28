// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#define _GNU_SOURCE
#include <signal.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pal/net/dns.h>
#include <pal/memory.h>
#include <HAPPlatform.h>

struct pal_dns_req_ctx {
    bool iscancel;
    pal_dns_response_cb cb;
    void *arg;
    struct gaicb gaicb;
    struct addrinfo hint;
    char hostname[0];
};

static const HAPLogObject dns_log_obj = {
    .subsystem = kHAPPlatform_LogSubsystem,
    .category = "dns",
};

static const int pal_dns_af_mapping[] = {
    [PAL_ADDR_FAMILY_UNSPEC] = AF_UNSPEC,
    [PAL_ADDR_FAMILY_IPV4] = AF_INET,
    [PAL_ADDR_FAMILY_IPV6] = AF_INET6
};

static void pal_dns_destroy_req_ctx(pal_dns_req_ctx *ctx) {
    if (ctx->gaicb.ar_result) {
        freeaddrinfo(ctx->gaicb.ar_result);
    }
    pal_mem_free(ctx);
}

static void pal_dns_req_ctx_schedule(void* _Nullable context, size_t contextSize) {
    HAPPrecondition(context);
    struct pal_dns_req_ctx *ctx = *(struct pal_dns_req_ctx **)context;

    const char *addr = NULL;
    int ret = gai_error(&ctx->gaicb);
    if (ret) {
        if (ret == EAI_INPROGRESS) {
            return;
        }
        HAPLogError(&dns_log_obj, "%s: resolve failed: %s.", __func__, gai_strerror(ret));
        goto done;
    }

    char buf[128];
    struct addrinfo *result = ctx->gaicb.ar_result;
    switch (result->ai_addr->sa_family) {
    case AF_INET: {
        struct sockaddr_in *in = (struct sockaddr_in *)result->ai_addr;
        addr = inet_ntop(AF_INET, &in->sin_addr, buf, sizeof(buf));
    } break;
    case AF_INET6: {
        struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)result->ai_addr;
        addr = inet_ntop(AF_INET6, &in6->sin6_addr, buf, sizeof(buf));
    } break;
    }

done:
    if (!ctx->iscancel) {
        ctx->cb(addr, ctx->arg);
    }
    pal_dns_destroy_req_ctx(ctx);
}

static void pal_dns_req_ctx_notify(__sigval_t sigev_value) {
    struct pal_dns_req_ctx *ctx = sigev_value.sival_ptr;
    HAPAssert(HAPPlatformRunLoopScheduleCallback(pal_dns_req_ctx_schedule,
        &ctx, sizeof(&ctx)) == kHAPError_None);
}

pal_dns_req_ctx *pal_dns_start_request(const char *hostname, pal_addr_family af,
    pal_dns_response_cb response_cb, void *arg) {
    HAPPrecondition(hostname);
    HAPPrecondition(af >= PAL_ADDR_FAMILY_UNSPEC && af <= PAL_ADDR_FAMILY_IPV6);
    HAPPrecondition(response_cb);

    size_t namelen = strlen(hostname);
    struct pal_dns_req_ctx *ctx = pal_mem_calloc(sizeof(*ctx) + namelen);
    if (!ctx) {
        HAPLogError(&dns_log_obj, "%s: Failed to alloc memory.", __func__);
        return NULL;
    }
    memcpy(ctx->hostname, hostname, namelen + 1);
    ctx->cb = response_cb;
    ctx->arg = arg;
    ctx->gaicb.ar_name = ctx->hostname;
    ctx->gaicb.ar_request = &ctx->hint;
    ctx->hint.ai_family = pal_dns_af_mapping[af];
    ctx->hint.ai_flags = AI_ADDRCONFIG;
    struct gaicb *cbs[] = { &ctx->gaicb };
    struct sigevent sigevent = {
        .sigev_notify = SIGEV_THREAD,
        .sigev_notify_function = pal_dns_req_ctx_notify,
        .sigev_value.sival_ptr = ctx,
    };
    int ret = getaddrinfo_a(GAI_NOWAIT, cbs, HAPArrayCount(cbs), &sigevent);
    if (ret) {
        HAPLogError(&dns_log_obj, "%s: getaddrinfo_a() failed: %s.", __func__, gai_strerror(ret));
        goto err;
    }

    return ctx;

err:
    pal_mem_free(ctx);
    return NULL;
}

void pal_dns_cancel_request(pal_dns_req_ctx *ctx) {
    HAPPrecondition(!ctx->iscancel);
    ctx->iscancel = true;
    gai_cancel(&ctx->gaicb);
}
