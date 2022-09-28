// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_DNS_H_
#define PLATFORM_INCLUDE_PAL_DNS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <pal/net.h>

/**
 * DNS resolve request context.
 */
typedef struct pal_dns_req_ctx pal_dns_req_ctx;

/**
 * A callback called when the response is received.
 *
 * @param err Error message. NULL on success, error message on failure.
 * @param addr The resolved address.
 * @param af Address family.
 * @param arg The last paramter of pal_dns_start_request().
 */
typedef void (*pal_dns_response_cb)(const char *err, const char *addr, pal_net_addr_family af, void *arg);

/**
 * Initialize DNS module.
 */
void pal_dns_init();

/**
 * De-initialize DNS module.
 */
void pal_dns_deinit();

/**
 * Start a DNS resolve request.
 *
 * @param hostname Host name.
 * @param af Address family.
 * @param response_cb A callback called when the response is received.
 * @param arg The value to be passed as the last argument to @p response_cb.
 * @return the request context on success.
 * @return NULL on failure.
 */
pal_dns_req_ctx *pal_dns_start_request(const char *hostname, pal_net_addr_family af,
    pal_dns_response_cb response_cb, void *arg);

/**
 * Cancel the DNS resolve request.
 * 
 * @param ctx DNS resolve request context.
 */
void pal_dns_cancel_request(pal_dns_req_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_DNS_H_
