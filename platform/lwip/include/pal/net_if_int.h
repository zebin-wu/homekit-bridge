// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_LWIP_INCLUDE_PAL_NET_IF_INT_H
#define PLATFORM_LWIP_INCLUDE_PAL_NET_IF_INT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize network interface module.
 */
void pal_net_if_init();

/**
 * De-initialize network interface module.
 */
void pal_net_if_deinit();

#endif  // PLATFORM_LWIP_INCLUDE_PAL_NET_IF_INT_H
