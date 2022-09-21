// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_NET_ADDR_H_
#define PLATFORM_INCLUDE_PAL_NET_ADDR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <pal/types.h>

/**
 * Address family.
 */
HAP_ENUM_BEGIN(uint8_t, pal_addr_family) {
    PAL_ADDR_FAMILY_UNSPEC,      /**< Unspecific. */
    PAL_ADDR_FAMILY_INET,        /**< IPv4. */
    PAL_ADDR_FAMILY_INET6,       /**< IPv6. */
} HAP_ENUM_END(uint8_t, pal_addr_family);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_NET_ADDR_H_
