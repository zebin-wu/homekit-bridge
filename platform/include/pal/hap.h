// Copyright (c) 2021-2022 KNpTrue and homekit-bridge contributors
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
 * Generate setup code, setup info and setup ID, and put them in the key-value store.
 */
void pal_hap_acc_setup_gen(HAPPlatformKeyValueStoreRef kv_store);

#if IP
/**
 * Initialize IP.
 */
void pal_hap_init_ip(HAPAccessoryServerOptions *options, size_t readable_cnt, size_t writable_cnt, size_t notify_cnt);

/**
 * Deinitialize IP.
 */
void pal_hap_deinit_ip(HAPAccessoryServerOptions *options);
#endif  // IP

#if BLE
/**
 * Initialize BLE.
 */
void pal_hap_init_ble(HAPAccessoryServerOptions *options, size_t attribute_cnt);

/**
 * Deinitialize BLE.
 */
void pal_hap_deinit_ble(HAPAccessoryServerOptions *options);
#endif  // BLE

#ifdef __cplusplus
}
#endif

#endif  // PLATFORM_INCLUDE_PAL_HAP_H_
