// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lwip/dns.h>
#include <esp_event.h>
#include <pal/dns.h>
#include <pal/mem.h>
#include <HAPPlatform.h>

#if LWIP_DNS != 1
#error Please enable LWIP_DNS
#endif /* LWIP_DNS */

#if LWIP_IPV6 != 1
#error Please enable LWIP_IPV6
#endif /* LWIP_IPV6 */

#define PAL_DNS_RESPONSE_EVENT_ID 1
ESP_EVENT_DEFINE_BASE(PAL_DNS_EVENTS);

struct pal_dns_req_ctx {
    bool iscancel;
    bool found;
    pal_dns_response_cb cb;
    void *arg;
    ip_addr_t addr;
};

static const HAPLogObject dns_log_obj = {
    .subsystem = kHAPPlatform_LogSubsystem,
    .category = "dns",
};

static const int pal_dns_af_mapping[] = {
    [PAL_NET_ADDR_FAMILY_UNSPEC] = LWIP_DNS_ADDRTYPE_DEFAULT,
    [PAL_NET_ADDR_FAMILY_INET] = LWIP_DNS_ADDRTYPE_IPV4,
    [PAL_NET_ADDR_FAMILY_INET6] = LWIP_DNS_ADDRTYPE_IPV6
};

static void pal_dns_response(void* _Nullable context, size_t contextSize) {
    pal_dns_req_ctx *ctx = *(pal_dns_req_ctx **)context;

    if (ctx->iscancel) {
        pal_mem_free(ctx);
        return;
    }

    pal_dns_response_cb cb = ctx->cb;
    void *arg = ctx->arg;
    const char *addr = NULL;
    pal_err err = PAL_ERR_OK;
    pal_net_addr_family af = PAL_NET_ADDR_FAMILY_UNSPEC;
    if (!ctx->found) {
        err = PAL_ERR_NOT_FOUND;
        goto done;
    }

    switch (IP_GET_TYPE(&ctx->addr)) {
    case IPADDR_TYPE_V4:
        af = PAL_NET_ADDR_FAMILY_INET;
        break;
    case IPADDR_TYPE_V6:
        af = PAL_NET_ADDR_FAMILY_INET6;
        break;
    }

    char buf[128];
    addr = ipaddr_ntoa_r(&ctx->addr, buf, sizeof(buf));
    if (!addr) {
        err = PAL_ERR_INVALID_ARG;
    }

done:
    pal_mem_free(ctx);
    cb(err, addr, af, arg);
}

void pal_dns_event_handler(void* event_handler_arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {
    HAPPrecondition(event_base == PAL_DNS_EVENTS);
    HAPPrecondition(event_id == PAL_DNS_RESPONSE_EVENT_ID);

    HAPAssert(HAPPlatformRunLoopScheduleCallback(pal_dns_response, event_data, sizeof(void *)) == kHAPError_None);
}

// This function is called in the tcp/ip thread, if function HAPPlatformRunLoopScheduleCallback
// is called directly here, it will cause deadlock.
static void pal_dns_found_cb(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    pal_dns_req_ctx *ctx = callback_arg;
    HAPAssert(!ctx->found);

    if (ipaddr) {
        ctx->found = true;
        ctx->addr = *ipaddr;
    }
    ESP_ERROR_CHECK(esp_event_post(PAL_DNS_EVENTS, PAL_DNS_RESPONSE_EVENT_ID, &ctx, sizeof(ctx), 0));
}

void pal_dns_init() {
    ESP_ERROR_CHECK(esp_event_handler_register(PAL_DNS_EVENTS, ESP_EVENT_ANY_ID, pal_dns_event_handler, NULL));
}

void pal_dns_deinit() {
    ESP_ERROR_CHECK(esp_event_handler_unregister(PAL_DNS_EVENTS, ESP_EVENT_ANY_ID, pal_dns_event_handler));
}

pal_dns_req_ctx *pal_dns_start_request(const char *hostname, pal_net_addr_family af,
    pal_dns_response_cb response_cb, void *arg) {
    HAPPrecondition(hostname);
    HAPPrecondition(af <= PAL_NET_ADDR_FAMILY_INET6);
    HAPPrecondition(response_cb);

    pal_dns_req_ctx *ctx = pal_mem_calloc(1, sizeof(*ctx));
    if (!ctx) {
        HAPLogError(&dns_log_obj, "%s: Failed to alloc memory.", __func__);
        return NULL;
    }

    ctx->cb = response_cb;
    ctx->arg = arg;

    err_t err = dns_gethostbyname_addrtype(hostname, &ctx->addr,
        pal_dns_found_cb, ctx, pal_dns_af_mapping[af]);
    switch (err) {
    case ERR_OK:
        ctx->found = true;
        HAPAssert(HAPPlatformRunLoopScheduleCallback(pal_dns_response, &ctx, sizeof(ctx)) == kHAPError_None);
        break;
    case ERR_INPROGRESS:
        break;
    default:
        HAPLogError(&dns_log_obj, "dns_gethostbyname_addrtype() returned %d", err);
        pal_mem_free(ctx);
        ctx = NULL;
        break;
    }

    return ctx;
}

void pal_dns_cancel_request(pal_dns_req_ctx *ctx) {
    HAPPrecondition(ctx);
    ctx->iscancel = true;
}
