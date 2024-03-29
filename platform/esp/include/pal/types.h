// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_ESP_INCLUDE_PAL_TYPES_H_
#define PLATFORM_ESP_INCLUDE_PAL_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <HAPBase.h>

/**
 * Opaque structure for cipher context.
 */
typedef HAP_OPAQUE(68) pal_cipher_ctx;

/**
 * Opaque structure for message-digest context.
 */
typedef HAP_OPAQUE(16) pal_md_ctx;

/**
 * Opaque structure for SSL context.
 */
typedef HAP_OPAQUE(432) pal_ssl_ctx;

/**
 * Opaque structure for socket object.
 */
typedef HAP_OPAQUE(112) pal_socket_obj;

/**
 * Opaque structure for network address.
 */
typedef HAP_OPAQUE(18) pal_net_addr;

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_ESP_INCLUDE_PAL_TYPES_H_
