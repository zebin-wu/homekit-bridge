// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_NET_IF_H_
#define PLATFORM_INCLUDE_PAL_NET_IF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pal/err.h>
#include <pal/types.h>
#include <pal/net_addr.h>

#define PAL_NET_IF_NAME_MAX_LEN 16
#define PAL_NET_IF_HWADDR_MAX_LEN 6

HAP_ENUM_BEGIN(uint8_t, pal_net_if_event) {
    PAL_NET_IF_EVENT_ADD,
    PAL_NET_IF_EVENT_DEL,
    PAL_NET_IF_EVENT_UP,
    PAL_NET_IF_EVENT_DOWN,
    PAL_NET_IF_EVENT_GOT_ADDR,
    PAL_NET_IF_EVENT_LOST_ADDR,

    PAL_NET_IF_EVENT_COUNT,
} HAP_ENUM_END(uint8_t, pal_net_if_event);

/**
 * Opaque structure for network interface.
 */
typedef struct pal_net_if pal_net_if;

/**
 * Initialize network interface module.
 */
void pal_net_if_init();

/**
 * De-initialize network interface module.
 */
void pal_net_if_deinit();

pal_err pal_net_if_foreach(pal_err (*func)(pal_net_if *netif, void *arg), void *arg);

pal_net_if *pal_net_if_find(const char *name);

pal_err pal_net_if_get_name(pal_net_if *netif, char buf[PAL_NET_IF_NAME_MAX_LEN]);

pal_err pal_net_if_get_hw_addr(pal_net_if *netif, uint8_t addr[PAL_NET_IF_HWADDR_MAX_LEN]);

pal_err pal_net_if_get_ipv4_addr(pal_net_if *netif, pal_net_addr *addr);

pal_err pal_net_if_get_ipv4_netmask(pal_net_if *netif, pal_net_addr *addr);

pal_err pal_net_if_ipv6_addr_foreach(pal_net_if *netif,
    pal_err (*func)(pal_net_if *netif, pal_net_addr *addr, void *arg), void *arg);

typedef void (*pal_net_if_event_cb)(pal_net_if *netif, pal_net_if_event event, void *arg);

void pal_net_if_register_event(pal_net_if *netif, pal_net_if_event event, pal_net_if_event_cb *event_cb);

void pal_net_if_unregister_event(pal_net_if *netif, pal_net_if_event event, pal_net_if_event_cb *event_cb);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_NET_IF_H_
