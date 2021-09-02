// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <pal/hap.h>
#include <pal/memory.h>

#include <HAPAccessorySetup.h>
#include <HAPPlatformKeyValueStore+SDKDomains.h>

#define PREFERRED_ADVERTISING_INTERVAL (HAPBLEAdvertisingIntervalCreateFromMilliseconds(417.5f))

void pal_hap_acc_setup_gen(HAPPlatformKeyValueStoreRef kv_store) {
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

#if IP
void pal_hap_init_ip(HAPAccessoryServerOptions *options, size_t attribute_cnt) {
    // Prepare accessory server storage.
    static HAPIPSession ipSessions[PAL_HAP_IP_SESSION_STORAGE_NUM_ELEMENTS];
    static uint8_t ipInboundBuffers[HAPArrayCount(ipSessions)][PAL_HAP_IP_SESSION_STORAGE_INBOUND_BUFSIZE];
    static uint8_t ipOutboundBuffers[HAPArrayCount(ipSessions)][PAL_HAP_IP_SESSION_STORAGE_OUTBOUND_BUFSIZE];
    for (size_t i = 0; i < HAPArrayCount(ipSessions); i++) {
        ipSessions[i].inboundBuffer.bytes = ipInboundBuffers[i];
        ipSessions[i].inboundBuffer.numBytes = sizeof ipInboundBuffers[i];
        ipSessions[i].outboundBuffer.bytes = ipOutboundBuffers[i];
        ipSessions[i].outboundBuffer.numBytes = sizeof ipOutboundBuffers[i];
        ipSessions[i].eventNotifications = pal_mem_alloc(sizeof(HAPIPEventNotificationRef) * attribute_cnt);
        HAPAssert(ipSessions[i].eventNotifications);
        ipSessions[i].numEventNotifications = attribute_cnt;
    }
    HAPIPReadContextRef *ipReadContexts = pal_mem_alloc(sizeof(HAPIPReadContextRef) * attribute_cnt);
    HAPAssert(ipReadContexts);
    HAPIPWriteContextRef *ipWriteContexts = pal_mem_alloc(sizeof(HAPIPWriteContextRef) * attribute_cnt);
    HAPAssert(ipWriteContexts);
    static uint8_t ipScratchBuffer[PAL_HAP_IP_SESSION_STORAGE_SCRATCH_BUFSIZE];
    static HAPIPAccessoryServerStorage ipAccessoryServerStorage = {
        .sessions = ipSessions,
        .numSessions = HAPArrayCount(ipSessions),
        .scratchBuffer = { .bytes = ipScratchBuffer, .numBytes = sizeof ipScratchBuffer }
    };
    ipAccessoryServerStorage.readContexts = ipReadContexts;
    ipAccessoryServerStorage.numReadContexts = attribute_cnt;
    ipAccessoryServerStorage.writeContexts = ipWriteContexts;
    ipAccessoryServerStorage.numWriteContexts = attribute_cnt;

    options->ip.transport = &kHAPAccessoryServerTransport_IP;
    options->ip.accessoryServerStorage = &ipAccessoryServerStorage;
}

void pal_hap_deinit_ip(HAPAccessoryServerOptions *options) {
    HAPIPAccessoryServerStorage *storage = options->ip.accessoryServerStorage;
    for (size_t i = 0; i < storage->numSessions; i++) {
        pal_mem_free(storage->sessions + i);
    }
    pal_mem_free(storage->readContexts);
    pal_mem_free(storage->writeContexts);
}
#endif

#if BLE
void pal_hap_init_ble(HAPAccessoryServerOptions *options, size_t attribute_cnt) {
    static HAPBLESessionCacheElementRef sessionCacheElements[kHAPBLESessionCache_MinElements];
    static HAPSessionRef session;
    static uint8_t procedureBytes[2048];
    static HAPBLEProcedureRef procedures[1];
    HAPBLEGATTTableElementRef *gattTableElements =
        pal_mem_alloc(sizeof(HAPBLEGATTTableElementRef) * attribute_cnt);
    HAPAssert(gattTableElements);

    static HAPBLEAccessoryServerStorage bleAccessoryServerStorage = {
        .sessionCacheElements = sessionCacheElements,
        .numSessionCacheElements = HAPArrayCount(sessionCacheElements),
        .session = &session,
        .procedures = procedures,
        .numProcedures = HAPArrayCount(procedures),
        .procedureBuffer = { .bytes = procedureBytes, .numBytes = sizeof procedureBytes }
    };
    bleAccessoryServerStorage.gattTableElements = gattTableElements;
    bleAccessoryServerStorage.numGATTTableElements = attribute_cnt;

    options->ble.transport = &kHAPAccessoryServerTransport_BLE;
    options->ble.accessoryServerStorage = &bleAccessoryServerStorage;
    options->ble.preferredAdvertisingInterval = PREFERRED_ADVERTISING_INTERVAL;
    options->ble.preferredNotificationDuration = kHAPBLENotification_MinDuration;
}

void pal_hap_deinit_ble(HAPAccessoryServerOptions *options) {
    pal_mem_free(options->ble.accessoryServerStorage->gattTableElements);
}
#endif
