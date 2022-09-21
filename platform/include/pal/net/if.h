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

#include <stdio.h>
#include <pal/err.h>
#include <pal/net/addr.h>

typedef struct pal_netif pal_netif;

typedef enum pal_netif_event {
    PAL_NETIF_ADD,
    PAL_NETIF_DEL,
    PAL_NETIF_UP,
    PAL_NETIF_DOWN,
    PAL_NETIF_GOT_ADDR,
    PAL_NETIF_LOST_ADDR,
} pal_netif_event;

pal_netif *pal_netif_iter(pal_netif *netif);

const char *pal_netif_get_name(pal_netif *netif);

const char *pal_netif_get_mac(pal_netif *netif);

pal_err pal_netif_get_addr(pal_netif *netif, char *buf, size_t len);

bool pal_netif_is_up(pal_netif *netif);

typedef void (*pal_netif_event_cb)(pal_netif *netif, pal_netif_event event, void *arg);

void pal_netif_event_register(pal_netif *netif, pal_netif_event event, pal_netif_event_cb *event_cb);

void pal_netif_event_unregister(pal_netif *netif, pal_netif_event event, pal_netif_event_cb *event_cb);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_NET_IF_H_
