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
#include <sys/queue.h>
#include <pal/net/dns.h>
#include <pal/mem.h>
#include <HAPPlatform.h>

typedef struct pal_dns_req_ctx {
    bool cancel;
    pal_dns_response_cb cb;
    void *arg;
    struct addrinfo hint;
    struct addrinfo *result;
    int ret;
    pthread_t tid;
    LIST_ENTRY(pal_dns_req_ctx) list_entry;
    char hostname[0];
} pal_dns_req_ctx;

static const HAPLogObject dns_log_obj = {
    .subsystem = kHAPPlatform_LogSubsystem,
    .category = "dns",
};

static const int pal_dns_af_mapping[] = {
    [PAL_ADDR_FAMILY_UNSPEC] = AF_UNSPEC,
    [PAL_ADDR_FAMILY_INET] = AF_INET,
    [PAL_ADDR_FAMILY_INET6] = AF_INET6
};

static bool ginited;
static LIST_HEAD(, pal_dns_req_ctx) greq_ctx_list_head;

static void pal_dns_destroy_resolve_ctx(struct pal_dns_req_ctx *ctx) {
    if (ctx->result) {
        freeaddrinfo(ctx->result);
        ctx->result = NULL;
    }
    LIST_REMOVE(ctx, list_entry);
    pal_mem_free(ctx);
}

static void pal_dns_req_ctx_schedule(void* _Nullable context, size_t contextSize) {
    HAPPrecondition(context);
    struct pal_dns_req_ctx *ctx = *(struct pal_dns_req_ctx **)context;

    pthread_join(ctx->tid, NULL);

    if (ctx->cancel) {
        pal_dns_destroy_resolve_ctx(ctx);
        return;
    }

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
        af = PAL_ADDR_FAMILY_INET;
    } break;
    case AF_INET6: {
        struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)result->ai_addr;
        addr = inet_ntop(AF_INET6, &in6->sin6_addr, buf, sizeof(buf));
        af = PAL_ADDR_FAMILY_INET6;
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
    HAPPrecondition(!ginited);
    LIST_INIT(&greq_ctx_list_head);
    ginited = true;
}

void pal_dns_deinit() {
    HAPPrecondition(ginited);
    for (struct pal_dns_req_ctx *t = LIST_FIRST(&greq_ctx_list_head); t;) {
        struct pal_dns_req_ctx *cur = t;
        t = LIST_NEXT(t, list_entry);
        pthread_join(cur->tid, NULL);
        pal_dns_destroy_resolve_ctx(cur);
    }
    ginited = false;
}

static void *pal_dns_resolve(void *arg) {
    struct pal_dns_req_ctx *ctx = arg;

    ctx->ret = getaddrinfo(ctx->hostname, NULL, &ctx->hint, &ctx->result);
    HAPAssert(HAPPlatformRunLoopScheduleCallback(pal_dns_req_ctx_schedule, &ctx, sizeof(ctx)) == kHAPError_None);
    return NULL;
}

pal_dns_req_ctx *pal_dns_start_request(const char *hostname, pal_addr_family af,
    pal_dns_response_cb response_cb, void *arg) {
    HAPPrecondition(ginited);
    HAPPrecondition(hostname);
    HAPPrecondition(af >= PAL_ADDR_FAMILY_UNSPEC && af <= PAL_ADDR_FAMILY_INET6);
    HAPPrecondition(response_cb);

    size_t namelen = strlen(hostname);
    pal_dns_req_ctx *ctx = pal_mem_alloc(sizeof(*ctx) + namelen + 1);
    if (!ctx) {
        HAPLogError(&dns_log_obj, "%s: Failed to alloc memory.", __func__);
        return NULL;
    }
    memcpy(ctx->hostname, hostname, namelen);
    ctx->hostname[namelen] = '\0';
    ctx->cb = response_cb;
    ctx->arg = arg;
    memset(&ctx->hint, 0, sizeof(ctx->hint));
    ctx->hint.ai_family = pal_dns_af_mapping[af];
    ctx->hint.ai_flags = AI_ADDRCONFIG;
    ctx->result = NULL;
    ctx->cancel = false;

    int ret = pthread_create(&ctx->tid, NULL, pal_dns_resolve, ctx);
    if (ret) {
        HAPLogError(&dns_log_obj, "%s: Failed to create a request thread.", __func__);
        pal_mem_free(ctx);
        return NULL;
    }

    LIST_INSERT_HEAD(&greq_ctx_list_head, ctx, list_entry);
    return ctx;
}

void pal_dns_cancel_request(pal_dns_req_ctx *ctx) {
    HAPPrecondition(ginited);
    HAPPrecondition(ctx);
    ctx->cancel = true;
}
