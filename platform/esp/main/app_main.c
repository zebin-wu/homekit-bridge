// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.
//
// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cmd_system.h>

#include <app.h>

#define IP 1

#include <HAP.h>
#include <HAPAccessorySetup.h>
#include <HAPPlatform+Init.h>
#include <HAPPlatformAccessorySetup+Init.h>
#include <HAPPlatformBLEPeripheralManager+Init.h>
#include <HAPPlatformKeyValueStore+Init.h>
#include <HAPPlatformKeyValueStore+SDKDomains.h>
#include <HAPPlatformMFiTokenAuth+Init.h>
#include <HAPPlatformRunLoop+Init.h>
#if IP
#include <HAPPlatformServiceDiscovery+Init.h>
#include <HAPPlatformTCPStreamManager+Init.h>
#endif
#if HAVE_MFI_HW_AUTH
#include <HAPPlatformMFiHWAuth+Init.h>
#endif

#include "app_wifi.h"
#include "app_console.h"
#include "app_spiffs.h"

#define APP_MAIN_TASK_STACKSIZE 8 * 1024
#define APP_MAIN_TASK_PRIORITY 6

static bool requested_factory_reset = false;
static bool clear_pairings = false;

#define PREFERRED_ADVERTISING_INTERVAL (HAPBLEAdvertisingIntervalCreateFromMilliseconds(417.5f))

/**
 * Global platform objects.
 * Only tracks objects that will be released in deinit_platform.
 */
static struct {
    HAPPlatformKeyValueStore keyValueStore;
    HAPAccessoryServerOptions hapAccessoryServerOptions;
    HAPPlatform hapPlatform;
    HAPAccessoryServerCallbacks hapAccessoryServerCallbacks;

    /* Client context pointer. Will be passed to callbacks. */
    void* _Nullable context;

#if HAVE_NFC
    HAPPlatformAccessorySetupNFC setupNFC;
#endif

#if IP
    HAPPlatformTCPStreamManager tcpStreamManager;
#endif

#if HAVE_MFI_HW_AUTH
    HAPPlatformMFiHWAuth mfiHWAuth;
#endif
    HAPPlatformMFiTokenAuth mfiTokenAuth;
} platform;

/**
 * HomeKit accessory server that hosts the accessory.
 */
static HAPAccessoryServerRef accessoryServer;

static void handle_update_state(HAPAccessoryServerRef* _Nonnull server, void* _Nullable context);

/**
 * Generate setup code, setup info and setup ID, and put them in the key-value store.
 */
static void accessory_setup_gen() {
    bool found;
    size_t numBytes;

    // Setup code.
    HAPSetupCode setupCode;
    HAPAssert(HAPPlatformKeyValueStoreGet(&platform.keyValueStore,
            kSDKKeyValueStoreDomain_Provisioning,
            kSDKKeyValueStoreKey_Provisioning_SetupCode,
            &setupCode, sizeof(setupCode), &numBytes,
            &found) == kHAPError_None);
    if (!found) {
        HAPAccessorySetupGenerateRandomSetupCode(&setupCode);
        HAPAssert(HAPPlatformKeyValueStoreSet(&platform.keyValueStore,
            kSDKKeyValueStoreDomain_Provisioning,
            kSDKKeyValueStoreKey_Provisioning_SetupCode,
            &setupCode, sizeof(setupCode)) == kHAPError_None);
    }

    // Setup info.
    HAPSetupInfo setupInfo;
    HAPAssert(HAPPlatformKeyValueStoreGet(&platform.keyValueStore,
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
        HAPAssert(HAPPlatformKeyValueStoreSet(&platform.keyValueStore,
            kSDKKeyValueStoreDomain_Provisioning,
            kSDKKeyValueStoreKey_Provisioning_SetupInfo,
            &setupInfo, sizeof(setupInfo)) == kHAPError_None);
    }

    // Setup ID.
    HAPSetupID setupID;
    bool hasSetupID;
    HAPPlatformAccessorySetupLoadSetupID(platform.hapPlatform.accessorySetup, &hasSetupID, &setupID);
    if (!hasSetupID) {
        HAPAccessorySetupGenerateRandomSetupID(&setupID);
        HAPAssert(HAPPlatformKeyValueStoreSet(&platform.keyValueStore,
            kSDKKeyValueStoreDomain_Provisioning,
            kSDKKeyValueStoreKey_Provisioning_SetupID,
            &setupID, sizeof(setupID)) == kHAPError_None);
    }
}

/**
 * Initialize global platform objects.
 */
static void init_platform() {
    // Key-value store.
    HAPPlatformKeyValueStoreCreate(&platform.keyValueStore, &(const HAPPlatformKeyValueStoreOptions) {
        .part_name = "nvs",
        .namespace_prefix = "hap",
        .read_only = false
    });
    platform.hapPlatform.keyValueStore = &platform.keyValueStore;

    // Accessory setup manager. Depends on key-value store.
    static HAPPlatformAccessorySetup accessorySetup;
    HAPPlatformAccessorySetupCreate(
            &accessorySetup, &(const HAPPlatformAccessorySetupOptions) { .keyValueStore = &platform.keyValueStore });
    platform.hapPlatform.accessorySetup = &accessorySetup;

    // Generate setup code, setup info and setup ID.
    accessory_setup_gen();

    app_console_init();
    app_wifi_init();
    app_wifi_on();
    app_spiffs_init();

    app_wifi_register_cmd();
    register_system();

#if IP
    // TCP stream manager.
    HAPPlatformTCPStreamManagerCreate(&platform.tcpStreamManager, &(const HAPPlatformTCPStreamManagerOptions) {
        /* Listen on all available network interfaces. */
        .port = 0 /* Listen on unused port number from the ephemeral port range. */,
        .maxConcurrentTCPStreams = 9
    });

    // Service discovery.
    static HAPPlatformServiceDiscovery serviceDiscovery;
    HAPPlatformServiceDiscoveryCreate(&serviceDiscovery, &(const HAPPlatformServiceDiscoveryOptions) {
        0, /* Register services on all available network interfaces. */
    });
    platform.hapPlatform.ip.serviceDiscovery = &serviceDiscovery;
#endif

#if (BLE)
    // BLE peripheral manager. Depends on key-value store.
    static HAPPlatformBLEPeripheralManagerOptions blePMOptions = { 0 };
    blePMOptions.keyValueStore = &platform.keyValueStore;

    static HAPPlatformBLEPeripheralManager blePeripheralManager;
    HAPPlatformBLEPeripheralManagerCreate(&blePeripheralManager, &blePMOptions);
    platform.hapPlatform.ble.blePeripheralManager = &blePeripheralManager;
#endif

#if HAVE_MFI_HW_AUTH
    // Apple Authentication Coprocessor provider.
    HAPPlatformMFiHWAuthCreate(&platform.mfiHWAuth);
    platform.hapPlatform.authentication.mfiHWAuth = &platform.mfiHWAuth;
#endif

    // Software Token provider. Depends on key-value store.
    HAPPlatformMFiTokenAuthCreate(
            &platform.mfiTokenAuth,
            &(const HAPPlatformMFiTokenAuthOptions) { .keyValueStore = &platform.keyValueStore });

    // Run loop.
    HAPPlatformRunLoopCreate(&(const HAPPlatformRunLoopOptions) { .keyValueStore = &platform.keyValueStore });

    platform.hapAccessoryServerOptions.maxPairings = kHAPPairingStorage_MinElements;

    platform.hapPlatform.authentication.mfiTokenAuth =
            HAPPlatformMFiTokenAuthIsProvisioned(&platform.mfiTokenAuth) ? &platform.mfiTokenAuth : NULL;

    platform.hapAccessoryServerCallbacks.handleUpdatedState = handle_update_state;
    platform.hapAccessoryServerCallbacks.handleSessionAccept = app_accessory_server_handle_session_accept;
    platform.hapAccessoryServerCallbacks.handleSessionInvalidate = app_accessory_server_handle_session_invalidate;
}

/**
 * Deinitialize global platform objects.
 */
static void deinit_platform() {
#if HAVE_MFI_HW_AUTH
    // Apple Authentication Coprocessor provider.
    HAPPlatformMFiHWAuthRelease(&platform.mfiHWAuth);
#endif

#if IP
    // TCP stream manager.
    HAPPlatformTCPStreamManagerRelease(&platform.tcpStreamManager);
#endif

    // Run loop.
    HAPPlatformRunLoopRelease();
}

/**
 * Restore platform specific factory settings.
 */
void restore_platform_factory_settings(void) {
}

/**
 * Either simply passes State handling to app, or processes Factory Reset.
 */
void handle_update_state(HAPAccessoryServerRef* _Nonnull server, void* _Nullable context) {
    switch (HAPAccessoryServerGetState(server)) {
    case kHAPAccessoryServerState_Idle:
        if (requested_factory_reset) {
            HAPPrecondition(server);

            HAPError err;

            HAPLogInfo(&kHAPLog_Default, "A factory reset has been requested.");

            // Purge app state.
            err = HAPPlatformKeyValueStorePurgeDomain(&platform.keyValueStore,
                ((HAPPlatformKeyValueStoreDomain) 0x00));
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPFatalError();
            }

            // Reset HomeKit state.
            err = HAPRestoreFactorySettings(&platform.keyValueStore);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPFatalError();
            }

            // Restore platform specific factory settings.
            restore_platform_factory_settings();

            // De-initialize App.
            app_release();

            requested_factory_reset = false;

            // Re-initialize App.
            app_create(server, &platform.keyValueStore);

            // Restart accessory server.
            app_accessory_server_start();
            return;
        } else if (clear_pairings) {
            HAPError err;
            err = HAPRemoveAllPairings(&platform.keyValueStore);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                HAPFatalError();
            }
            app_accessory_server_start();
        }
        break;
    case kHAPAccessoryServerState_Running:
        app_accessory_server_handle_update_state(server, context);

        // Start the console.
        app_console_start();
        break;
    default:
        app_accessory_server_handle_update_state(server, context);
        break;
    }
}

#if IP
static void init_ip(size_t attribute_cnt) {
    // Prepare accessory server storage.
    static HAPIPSession ipSessions[kHAPIPSessionStorage_MinimumNumElements];
    static uint8_t ipInboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_MinimumInboundBufferSize];
    static uint8_t ipOutboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_MinimumOutboundBufferSize];
    for (size_t i = 0; i < HAPArrayCount(ipSessions); i++) {
        ipSessions[i].inboundBuffer.bytes = ipInboundBuffers[i];
        ipSessions[i].inboundBuffer.numBytes = sizeof ipInboundBuffers[i];
        ipSessions[i].outboundBuffer.bytes = ipOutboundBuffers[i];
        ipSessions[i].outboundBuffer.numBytes = sizeof ipOutboundBuffers[i];
        ipSessions[i].eventNotifications = malloc(sizeof(HAPIPEventNotificationRef) * attribute_cnt);
        HAPAssert(ipSessions[i].eventNotifications);
        ipSessions[i].numEventNotifications = attribute_cnt;
    }
    HAPIPReadContextRef *ipReadContexts = malloc(sizeof(HAPIPReadContextRef) * attribute_cnt);
    HAPAssert(ipReadContexts);
    HAPIPWriteContextRef *ipWriteContexts = malloc(sizeof(HAPIPWriteContextRef) * attribute_cnt);
    HAPAssert(ipWriteContexts);
    static uint8_t ipScratchBuffer[kHAPIPSession_MinimumScratchBufferSize];
    static HAPIPAccessoryServerStorage ipAccessoryServerStorage = {
        .sessions = ipSessions,
        .numSessions = HAPArrayCount(ipSessions),
        .scratchBuffer = { .bytes = ipScratchBuffer, .numBytes = sizeof ipScratchBuffer }
    };
    ipAccessoryServerStorage.readContexts = ipReadContexts;
    ipAccessoryServerStorage.numReadContexts = attribute_cnt;
    ipAccessoryServerStorage.writeContexts = ipWriteContexts;
    ipAccessoryServerStorage.numWriteContexts = attribute_cnt;

    platform.hapAccessoryServerOptions.ip.transport = &kHAPAccessoryServerTransport_IP;
    platform.hapAccessoryServerOptions.ip.accessoryServerStorage = &ipAccessoryServerStorage;

    platform.hapPlatform.ip.tcpStreamManager = &platform.tcpStreamManager;
}

static void deinit_ip(void) {
    HAPIPAccessoryServerStorage *storage = platform.hapAccessoryServerOptions.ip.accessoryServerStorage;
    for (size_t i = 0; i < storage->numSessions; i++) {
        free(storage->sessions + i);
    }
    free(storage->readContexts);
    free(storage->writeContexts);
}
#endif

#if BLE
static void init_ble(size_t attribute_cnt) {
    static HAPBLESessionCacheElementRef sessionCacheElements[kHAPBLESessionCache_MinElements];
    static HAPSessionRef session;
    static uint8_t procedureBytes[2048];
    static HAPBLEProcedureRef procedures[1];
    static HAPBLEGATTTableElementRef *gattTableElements =
        malloc(sizeof(HAPBLEGATTTableElementRef) * attribute_cnt);
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

    platform.hapAccessoryServerOptions.ble.transport = &kHAPAccessoryServerTransport_BLE;
    platform.hapAccessoryServerOptions.ble.accessoryServerStorage = &bleAccessoryServerStorage;
    platform.hapAccessoryServerOptions.ble.preferredAdvertisingInterval = PREFERRED_ADVERTISING_INTERVAL;
    platform.hapAccessoryServerOptions.ble.preferredNotificationDuration = kHAPBLENotification_MinDuration;
}

static void deinit_ble(void) {
    free(platform.hapAccessoryServerOptions.ble.accessoryServerStorage->gattTableElements);
}
#endif

void app_main_task(void *arg) {
    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);

    // Initialize global platform objects.
    init_platform();

    // Lua entry.
    size_t attribute_cnt = app_lua_entry(APP_SPIFFS_DIR_PATH, CONFIG_LUA_APP_ENTRY);
    HAPAssert(attribute_cnt);

#if IP
    init_ip(attribute_cnt);
#endif

#if BLE
    init_ble(attribute_cnt);
#endif

    // Perform Application-specific initalizations such as setting up callbacks
    // and configure any additional unique platform dependencies
    app_init(&platform.hapAccessoryServerOptions, &platform.hapPlatform,
        &platform.hapAccessoryServerCallbacks, &platform.context);

    // Initialize accessory server.
    HAPAccessoryServerCreate(
            &accessoryServer,
            &platform.hapAccessoryServerOptions,
            &platform.hapPlatform,
            &platform.hapAccessoryServerCallbacks,
            platform.context);

    // Create app object.
    app_create(&accessoryServer, &platform.keyValueStore);

    // Start accessory server for App.
    app_accessory_server_start();

    // Run main loop until explicitly stopped.
    HAPPlatformRunLoopRun();
    // Run loop stopped explicitly by calling function HAPPlatformRunLoopStop.

    // Cleanup.
    app_release();

    HAPAccessoryServerRelease(&accessoryServer);

    app_deinit();

#if IP
    deinit_ip();
#endif

#if BLE
    deinit_ble();
#endif

    // Close lua state.
    app_lua_close();

    deinit_platform();
}

void app_main() {
    xTaskCreate(app_main_task, "app", APP_MAIN_TASK_STACKSIZE,
        NULL, APP_MAIN_TASK_PRIORITY, NULL);
}
