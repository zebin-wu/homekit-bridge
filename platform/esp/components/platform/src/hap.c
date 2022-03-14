// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <pal/hap.h>
#include <pal/memory.h>

#include <HAPPlatform+Init.h>
#include <HAPAccessorySetup.h>
#include <HAPPlatformAccessorySetup+Init.h>
#include <HAPPlatformKeyValueStore+SDKDomains.h>
#include <HAPPlatformBLEPeripheralManager+Init.h>
#include <HAPPlatformKeyValueStore+Init.h>
#include <HAPPlatformMFiTokenAuth+Init.h>
#include <HAPPlatformLog+Init.h>
#include <HAPPlatformServiceDiscovery+Init.h>
#include <HAPPlatformTCPStreamManager+Init.h>

#if HAVE_MFI_HW_AUTH
#include <HAPPlatformMFiHWAuth+Init.h>
#endif

static bool ginited;

/**
 * Generate setup code, setup info and setup ID, and put them in the key-value store.
 */
static void pal_hap_acc_setup_gen(HAPPlatformKeyValueStoreRef kv_store) {
    bool found;
    size_t numBytes;

    // Setup code.
    HAPSetupCode setupCode;
    HAPAssert(HAPPlatformKeyValueStoreGet(kv_store,
            kSDKKeyValueStoreDomain_Provisioning,
            kSDKKeyValueStoreKey_Provisioning_SetupCode,
            &setupCode, sizeof(setupCode), &numBytes,
            &found) == kHAPError_None);
    if (!found) {
        HAPAccessorySetupGenerateRandomSetupCode(&setupCode);
        HAPAssert(HAPPlatformKeyValueStoreSet(kv_store,
            kSDKKeyValueStoreDomain_Provisioning,
            kSDKKeyValueStoreKey_Provisioning_SetupCode,
            &setupCode, sizeof(setupCode)) == kHAPError_None);
    }

    // Setup info.
    HAPSetupInfo setupInfo;
    HAPAssert(HAPPlatformKeyValueStoreGet(kv_store,
            kSDKKeyValueStoreDomain_Provisioning,
            kSDKKeyValueStoreKey_Provisioning_SetupInfo,
            &setupInfo, sizeof(setupInfo), &numBytes,
            &found) == kHAPError_None);
    if (!found) {
        HAPPlatformRandomNumberFill(setupInfo.salt, sizeof setupInfo.salt);
        const uint8_t srpUserName[] = "Pair-Setup";
        HAP_srp_verifier(
                setupInfo.verifier,
                setupInfo.salt,
                srpUserName,
                sizeof srpUserName - 1,
                (const uint8_t*) setupCode.stringValue,
                sizeof setupCode.stringValue - 1);
        HAPAssert(HAPPlatformKeyValueStoreSet(kv_store,
            kSDKKeyValueStoreDomain_Provisioning,
            kSDKKeyValueStoreKey_Provisioning_SetupInfo,
            &setupInfo, sizeof(setupInfo)) == kHAPError_None);
    }

    // Setup ID.
    HAPSetupID setupID;
    HAPAssert(HAPPlatformKeyValueStoreGet(kv_store,
            kSDKKeyValueStoreDomain_Provisioning,
            kSDKKeyValueStoreKey_Provisioning_SetupID,
            &setupID, sizeof(setupID), &numBytes,
            &found) == kHAPError_None);
    if (!found) {
        HAPAccessorySetupGenerateRandomSetupID(&setupID);
        HAPAssert(HAPPlatformKeyValueStoreSet(kv_store,
            kSDKKeyValueStoreDomain_Provisioning,
            kSDKKeyValueStoreKey_Provisioning_SetupID,
            &setupID, sizeof(setupID)) == kHAPError_None);
    }
}

void pal_hap_init_platform(HAPPlatform *platform) {
    HAPPrecondition(!ginited);
    HAPPrecondition(platform);

    // Key-value store.
    platform->keyValueStore = pal_mem_alloc(sizeof(HAPPlatformKeyValueStore));
    HAPAssert(platform->keyValueStore);
    HAPPlatformKeyValueStoreCreate(platform->keyValueStore, &(const HAPPlatformKeyValueStoreOptions) {
        .part_name = "nvs",
        .namespace_prefix = "hap",
        .read_only = false
    });

    // Generate setup code, setup info and setup ID.
    pal_hap_acc_setup_gen(platform->keyValueStore);

    // Accessory setup manager. Depends on key-value store.
    platform->accessorySetup = pal_mem_alloc(sizeof(HAPPlatformAccessorySetup));
    HAPAssert(platform->accessorySetup);
    HAPPlatformAccessorySetupCreate(
            platform->accessorySetup,
            &(const HAPPlatformAccessorySetupOptions) { .keyValueStore = platform->keyValueStore });

    // TCP stream manager.
    platform->ip.tcpStreamManager = pal_mem_alloc(sizeof(HAPPlatformTCPStreamManager));
    HAPAssert(platform->ip.tcpStreamManager);
    HAPPlatformTCPStreamManagerCreate(
            platform->ip.tcpStreamManager,
            &(const HAPPlatformTCPStreamManagerOptions) {
                    .port = kHAPNetworkPort_Any,  // Listen on unused port number from the ephemeral port range.
                    .maxConcurrentTCPStreams = PAL_HAP_IP_SESSION_STORAGE_NUM_ELEMENTS });

    // Service discovery.
    platform->ip.serviceDiscovery = pal_mem_alloc(sizeof(HAPPlatformServiceDiscovery));
    HAPAssert(platform->ip.serviceDiscovery);
    HAPPlatformServiceDiscoveryCreate(
            platform->ip.serviceDiscovery,
            &(const HAPPlatformServiceDiscoveryOptions) {
                    NULL, /* Default host name is "homekit-bridge". */
            });

#if HAVE_MFI_HW_AUTH
    // Apple Authentication Coprocessor provider.
    platform->authentication.mfiHWAuth = pal_mem_alloc(sizeof(HAPPlatformMFiHWAuth));
    HAPAssert(platform->authentication.mfiHWAuth);
    HAPPlatformMFiHWAuthCreate(platform->authentication.mfiHWAuth);
#endif

    // Software Token provider. Depends on key-value store.
    platform->authentication.mfiTokenAuth = pal_mem_alloc(sizeof(HAPPlatformMFiTokenAuth));
    HAPAssert(platform->authentication.mfiTokenAuth);
    HAPPlatformMFiTokenAuthCreate(
            platform->authentication.mfiTokenAuth,
            &(const HAPPlatformMFiTokenAuthOptions) { .keyValueStore = platform->keyValueStore });
    if (!HAPPlatformMFiTokenAuthIsProvisioned(platform->authentication.mfiTokenAuth)) {
        pal_mem_free(platform->authentication.mfiTokenAuth);
        platform->authentication.mfiTokenAuth = NULL;
    }

    ginited = true;
}

void pal_hap_deinit_platform(HAPPlatform *platform) {
    HAPPrecondition(ginited);
    HAPPrecondition(platform);

    if (platform->authentication.mfiTokenAuth) {
        pal_mem_free(platform->authentication.mfiTokenAuth);
    }
#if HAVE_MFI_HW_AUTH
    // Apple Authentication Coprocessor provider.
    HAPPlatformMFiHWAuthRelease(platform->authentication.mfiHWAuth);
    pal_mem_free(platform->authentication.mfiHWAuth);
#endif

    pal_mem_free(platform->ip.serviceDiscovery);

    // TCP stream manager.
    HAPPlatformTCPStreamManagerRelease(platform->ip.tcpStreamManager);
    pal_mem_free(platform->ip.tcpStreamManager);

    pal_mem_free(platform->accessorySetup);
    pal_mem_free(platform->keyValueStore);

    HAPRawBufferZero(platform, sizeof(*platform));
    ginited = false;
}

bool pal_hap_restore_factory_settings() {
    HAPPrecondition(!ginited);

    HAPPlatformKeyValueStore kv_store;
    HAPPlatformKeyValueStoreCreate(&kv_store, &(const HAPPlatformKeyValueStoreOptions) {
        .part_name = "nvs",
        .namespace_prefix = "hap",
        .read_only = false
    });

    return HAPRestoreFactorySettings(&kv_store) == kHAPError_None;
}
