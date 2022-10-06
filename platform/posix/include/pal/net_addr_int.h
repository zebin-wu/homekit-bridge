// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_POSIX_INCLUDE_NET_ADDR_INT_H_
#define PLATFORM_INCLUDE_PAL_POSIX_INCLUDE_NET_ADDR_INT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <arpa/inet.h>
#include <pal/types.h>
#include <pal/net_addr.h>

typedef struct pal_net_addr_int {
    pal_net_addr_family family;     /**< Address family. */
    union {
        struct in_addr in;      /**< IPv4. */
        struct in6_addr in6;    /**< IPv6. */
    } u;
} pal_net_addr_int;
HAP_STATIC_ASSERT(sizeof(pal_net_addr) >= sizeof(pal_net_addr_int), pal_net_addr_int);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_POSIX_INCLUDE_NET_ADDR_INT_H_
