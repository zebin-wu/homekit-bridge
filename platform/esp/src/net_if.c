// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <string.h>
#include <esp_event.h>
#include <lwip/netif.h>
#include <lwip/inet.h>
#include <sys/queue.h>
#include <pal/mem.h>
#include <pal/net_if.h>
#include <pal/net_addr_int.h>
#include <HAPPlatform.h>

#if !LWIP_NETIF_EXT_STATUS_CALLBACK
#error "LWIP_NETIF_EXT_STATUS_CALLBACK must enabled"
#endif

#define PAL_NET_IF_EVENT_ID 1
ESP_EVENT_DEFINE_BASE(PAL_NET_IF_EVENTS);

NETIF_DECLARE_EXT_CALLBACK(netif_callback)

struct event_cb_desc {
    pal_net_if *netif;
    pal_net_if_event_cb cb;
    void *arg;
    LIST_ENTRY(event_cb_desc) list_entry;
};

static LIST_HEAD(, event_cb_desc) gevent_cbs[PAL_NET_IF_EVENT_COUNT];

static void pal_net_if_call_event_handlers(pal_net_if *netif, pal_net_if_event event) {
    struct event_cb_desc *desc;
    LIST_FOREACH(desc, gevent_cbs + event, list_entry) {
        if (desc->netif && desc->netif != netif) {
            continue;
        }
        desc->cb(netif, event, desc->arg);
    }
}

struct pal_net_if_ext_callback_ctx {
    struct netif *netif;
    netif_nsc_reason_t reason;
    const netif_ext_callback_args_t *args;
};

static void pal_net_if_ext_callback_schedule(void* _Nullable context, size_t contextSize) {
    HAPPrecondition(context);
    HAPPrecondition(contextSize == sizeof(struct pal_net_if_ext_callback_ctx));

    struct pal_net_if_ext_callback_ctx *ctx = (struct pal_net_if_ext_callback_ctx *)context;
    struct netif *netif = ctx->netif;
    netif_nsc_reason_t reason = ctx->reason;
    const netif_ext_callback_args_t *args = ctx->args;

    if (reason & LWIP_NSC_NETIF_ADDED) {
        pal_net_if_call_event_handlers((pal_net_if *)netif, PAL_NET_IF_EVENT_ADDED);
    }
    if (reason & LWIP_NSC_NETIF_REMOVED) {
        pal_net_if_call_event_handlers((pal_net_if *)netif, PAL_NET_IF_EVENT_REMOVED);
    }
    if (reason & LWIP_NSC_STATUS_CHANGED) {
        pal_net_if_call_event_handlers((pal_net_if *)netif,
            args->status_changed.state ? PAL_NET_IF_EVENT_UP : PAL_NET_IF_EVENT_DOWN);
    }
    if (reason & LWIP_NSC_IPV4_ADDRESS_CHANGED) {
        pal_net_if_call_event_handlers((pal_net_if *)netif, PAL_NET_IF_EVENT_IPV4_ADDR_CHANGED);
    }
    if (reason & (LWIP_NSC_IPV6_ADDR_STATE_CHANGED | LWIP_NSC_IPV6_ADDR_STATE_CHANGED)) {
        pal_net_if_call_event_handlers((pal_net_if *)netif, PAL_NET_IF_EVENT_IPV6_ADDR_CHANGED);
    }
}

static void pal_net_if_event_handler(void* event_handler_arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {
    HAPPrecondition(event_base == PAL_NET_IF_EVENTS);
    HAPPrecondition(event_id == PAL_NET_IF_EVENT_ID);

    struct pal_net_if_ext_callback_ctx *ctx = event_data;

    HAPAssert(
        HAPPlatformRunLoopScheduleCallback(pal_net_if_ext_callback_schedule, ctx, sizeof(*ctx)) == kHAPError_None);
}

// This function is called in the tcp/ip thread, if function HAPPlatformRunLoopScheduleCallback
// is called directly here, it will cause deadlock.
static void pal_net_if_ext_callback(struct netif *netif,
    netif_nsc_reason_t reason, const netif_ext_callback_args_t *args) {
    struct pal_net_if_ext_callback_ctx ctx = {
        .netif = netif,
        .reason = reason,
        .args = args,
    };
    ESP_ERROR_CHECK(esp_event_post(PAL_NET_IF_EVENTS, PAL_NET_IF_EVENT_ID, &ctx, sizeof(ctx), 0));
}

void pal_net_if_init() {
    for (size_t i = 0; i < PAL_NET_IF_EVENT_COUNT; i++) {
        LIST_INIT(&gevent_cbs[i]);
    }

    netif_add_ext_callback(&netif_callback, pal_net_if_ext_callback);
    ESP_ERROR_CHECK(esp_event_handler_register(PAL_NET_IF_EVENTS, ESP_EVENT_ANY_ID, pal_net_if_event_handler, NULL));
}

void pal_net_if_deinit() {
    ESP_ERROR_CHECK(esp_event_handler_unregister(PAL_NET_IF_EVENTS, ESP_EVENT_ANY_ID, pal_net_if_event_handler));
    netif_remove_ext_callback(&netif_callback);

    for (size_t i = 0; i < PAL_NET_IF_EVENT_COUNT; i++) {
        for (struct event_cb_desc *next = LIST_FIRST(gevent_cbs + i); next;) {
            struct event_cb_desc *cur = next;
            next = LIST_NEXT(cur, list_entry);
            pal_mem_free(cur);
        }
    }
}

pal_err pal_net_if_foreach(pal_err (*func)(pal_net_if *netif, void *arg), void *arg) {
    HAPPrecondition(func);

    struct netif *netif = NULL;
    NETIF_FOREACH(netif) {
        pal_err err = func((pal_net_if *)netif, arg);
        if (err != PAL_ERR_OK) {
            return err;
        }
    }
    return PAL_ERR_OK;
}

pal_net_if *pal_net_if_find(const char *name) {
    HAPPrecondition(name && name[0]);

    return (pal_net_if *)netif_find(name);
}

pal_err pal_net_if_is_up(pal_net_if *netif, bool *up) {
    HAPPrecondition(netif);
    HAPPrecondition(up);

    *up = netif_is_up((struct netif *)netif);

    return PAL_ERR_OK;
}

pal_err pal_net_if_get_index(pal_net_if *netif, int *index) {
    HAPPrecondition(netif);
    HAPPrecondition(index);

    *index = netif_get_index((struct netif *)netif);
    return PAL_ERR_OK;
}

pal_err pal_net_if_get_name(pal_net_if *_netif, char buf[PAL_NET_IF_NAME_MAX_LEN]) {
    HAPPrecondition(_netif);
    HAPPrecondition(buf);

    struct netif *netif = (struct netif *)_netif;

    snprintf(buf, PAL_NET_IF_NAME_MAX_LEN, "%c%c%d", netif->name[0], netif->name[1], netif_get_index(netif));

    return PAL_ERR_OK;
}

pal_err pal_net_if_get_hw_addr(pal_net_if *_netif, uint8_t addr[PAL_NET_IF_HW_ADDR_LEN]) {
    HAPPrecondition(_netif);
    HAPPrecondition(addr);

    struct netif *netif = (struct netif *)_netif;

    memcpy(addr, netif->hwaddr, PAL_NET_IF_HW_ADDR_LEN);

    return PAL_ERR_OK;
}

pal_err pal_net_if_get_ipv4_addr(pal_net_if *netif, pal_net_addr *_addr) {
    HAPPrecondition(netif);
    HAPPrecondition(_addr);

    pal_net_addr_int *addr = (pal_net_addr_int *)_addr;

    addr->family = PAL_NET_ADDR_FAMILY_INET;
    inet_addr_from_ip4addr(&addr->u.in, netif_ip4_addr((struct netif *)netif));

    return PAL_ERR_OK;
}

pal_err pal_net_if_ipv6_addr_foreach(pal_net_if *_netif,
    pal_err (*func)(pal_net_if *netif, pal_net_addr *addr, void *arg), void *arg) {
    HAPPrecondition(_netif);
    HAPPrecondition(func);

    struct netif *netif = (struct netif *)_netif;

    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (!ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
            continue;
        }

        pal_net_addr_int addr;
        addr.family = PAL_NET_ADDR_FAMILY_INET6;
        inet6_addr_from_ip6addr(&addr.u.in6, netif_ip6_addr(netif, i));

        pal_err err = func((pal_net_if *)netif, (pal_net_addr *)&addr, arg);
        if (err != PAL_ERR_OK) {
            return err;
        }
    }

    return PAL_ERR_OK;
}

pal_err pal_net_if_register_event_callback(pal_net_if *netif, pal_net_if_event event,
    pal_net_if_event_cb event_cb, void *arg) {
    HAPPrecondition(event <= PAL_NET_IF_EVENT_END);
    HAPPrecondition(event_cb);

    struct event_cb_desc *desc = pal_mem_alloc(sizeof(*desc));
    if (!desc) {
        return PAL_ERR_ALLOC;
    }

    desc->netif = netif;
    desc->cb = event_cb;
    desc->arg = arg;
    LIST_INSERT_HEAD(gevent_cbs + event, desc, list_entry);

    return PAL_ERR_OK;
}

pal_err pal_net_if_unregister_event_callback(pal_net_if *netif, pal_net_if_event event,
    pal_net_if_event_cb event_cb, void *arg) {
    HAPPrecondition(event <= PAL_NET_IF_EVENT_END);
    HAPPrecondition(event_cb);

    struct event_cb_desc *desc;
    LIST_FOREACH(desc, gevent_cbs + event, list_entry) {
        if (desc->netif == netif && desc->cb == event_cb) {
            LIST_REMOVE(desc, list_entry);
            pal_mem_free(desc);
            return PAL_ERR_OK;
        }
    }

    return PAL_ERR_INVALID_ARG;
}
