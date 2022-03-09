// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#ifndef PLATFORM_INCLUDE_PAL_HAP_H_
#define PLATFORM_INCLUDE_PAL_HAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <HAP.h>

// Number of elements in a HAPIPSessionStorage.
#define PAL_HAP_IP_SESSION_STORAGE_NUM_ELEMENTS ((size_t) 5)

// Size for the inbound buffer of an IP session.
#define PAL_HAP_IP_SESSION_STORAGE_INBOUND_BUFSIZE ((size_t) 1500)

// Size for the outbound buffer of an IP session.
#define PAL_HAP_IP_SESSION_STORAGE_OUTBOUND_BUFSIZE ((size_t) 1500)

// Size for the scratch buffer of an IP session.
#define PAL_HAP_IP_SESSION_STORAGE_SCRATCH_BUFSIZE ((size_t) 1500)

/**
 * Initialize HAP platform structure.
 */
void pal_hap_init_platform(HAPPlatform *platform);

/**
 * De-initialize HAP platform structure.
 */
void pal_hap_deinit_platform(HAPPlatform *platform);

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_HAP_H_
