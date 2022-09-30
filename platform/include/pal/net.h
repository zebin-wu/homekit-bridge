// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_NET_H_
#define PLATFORM_INCLUDE_PAL_NET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pal/err.h>
#include <pal/types.h>

/**
 * Address family.
 */
HAP_ENUM_BEGIN(uint8_t, pal_net_addr_family) {
    PAL_NET_ADDR_FAMILY_UNSPEC,     /**< Unspecific. */
    PAL_NET_ADDR_FAMILY_INET,       /**< IPv4. */
    PAL_NET_ADDR_FAMILY_INET6,      /**< IPv6. */
} HAP_ENUM_END(uint8_t, pal_net_addr_family);

HAP_ENUM_BEGIN(uint8_t, pal_net_if_event) {
    PAL_NET_IF_EVENT_ADD,
    PAL_NET_IF_EVENT_DEL,
    PAL_NET_IF_EVENT_UP,
    PAL_NET_IF_EVENT_DOWN,
    PAL_NET_IF_EVENT_GOT_ADDR,
    PAL_NET_IF_EVENT_LOST_ADDR,
} HAP_ENUM_END(uint8_t, pal_net_if_event);

/**
 * Initialize a network address.
 *
 * @param addr The network address to initialize.
 * @param af Address family, must be either PAL_NET_ADDR_FAMILY_INET or PAL_NET_ADDR_FAMILY_INET6.
 * @param s The string of the address.
 *
 * @return PAL_ERR_OK on success.
 * @return PAL_ERR_INVALID_ARG means the string is invalid.
 */
pal_err pal_net_addr_init(pal_net_addr *addr, pal_net_addr_family af, const char *s);

/**
 * Get the address family of the network address.
 *
 * @param addr The pointer to the network address.
 * @return address family.
 */
pal_net_addr_family pal_net_addr_family_get(pal_net_addr *addr);

/**
 * Get the string of the network address.
 *
 * @param addr The pointer to the network address.
 * @param buf A buffer to hold the string.
 * @param buflen Length of @p buf.
 * @return the string of the network address.
 */
const char *pal_net_addr_string_get(pal_net_addr *addr, char *buf, size_t buflen);

pal_net_if *pal_net_if_get(const char *name);

pal_net_if *pal_net_if_iter(pal_net_if *netif);

const char *pal_net_if_name_get(pal_net_if *netif);

const char *pal_net_if_mac_get(pal_net_if *netif);

pal_err pal_net_if_addr_get(pal_net_if *netif, pal_net_addr_family af, pal_net_addr *addrs[], size_t *naddrs);

bool pal_net_if_is_up(pal_net_if *netif);

typedef void (*pal_net_if_event_cb)(pal_net_if *netif, pal_net_if_event event, void *arg);

void pal_net_if_event_register(pal_net_if *netif, pal_net_if_event event, pal_net_if_event_cb *event_cb);

void pal_net_if_event_unregister(pal_net_if *netif, pal_net_if_event event, pal_net_if_event_cb *event_cb);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_NET_H_
