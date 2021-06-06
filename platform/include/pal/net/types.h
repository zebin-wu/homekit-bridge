// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_NET_TYPES_H_
#define PLATFORM_INCLUDE_PAL_NET_TYPES_H_

/**
 * Network error numbers.
 */
typedef enum {
    PAL_NET_ERR_OK,  // No error.
    PAL_NET_ERR_UNKNOWN,  // Unknown.
} pal_net_err;

/**
 * Communication domain.
 */
typedef enum {
    PAL_NET_DOMAIN_INET,  // IPv4 Internet protocols
    PAL_NET_DOMAIN_INET6,  // IPv6 Internet protocols
} pal_net_domain;

#endif  // PLATFORM_INCLUDE_PAL_NET_TYPES_H_
