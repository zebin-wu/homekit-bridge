// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.
//
// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include <stdio.h>

#include <app.h>
#include <pal/hap.h>

#include <HAPPlatform+Init.h>
#include <HAPPlatformAccessorySetup+Init.h>
#include <HAPPlatformBLEPeripheralManager+Init.h>
#include <HAPPlatformKeyValueStore+Init.h>
#include <HAPPlatformMFiTokenAuth+Init.h>
#include <HAPPlatformRunLoop+Init.h>
#if IP
#include <HAPPlatformServiceDiscovery+Init.h>
#include <HAPPlatformTCPStreamManager+Init.h>
#endif

#if HAVE_MFI_HW_AUTH
#include <HAPPlatformMFiHWAuth+Init.h>
#endif

#ifndef BRIDGE_WORK_DIR
#error Please set the macro "BRIDGE_WORK_DIR"
#endif

/**
 * Global platform objects.
 * Only tracks objects that will be released in deinit_platform.
 */
static struct {
    HAPPlatformKeyValueStore keyValueStore;
    HAPPlatform hapPlatform;

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
 * Initialize global platform objects.
 */
static void init_platform() {
    // Key-value store.
    HAPPlatformKeyValueStoreCreate(
            &platform.keyValueStore, &(const HAPPlatformKeyValueStoreOptions) { .rootDirectory = ".HomeKitStore" });
    platform.hapPlatform.keyValueStore = &platform.keyValueStore;

    // Accessory setup manager. Depends on key-value store.
    static HAPPlatformAccessorySetup accessorySetup;
    HAPPlatformAccessorySetupCreate(
            &accessorySetup, &(const HAPPlatformAccessorySetupOptions) { .keyValueStore = &platform.keyValueStore });
    platform.hapPlatform.accessorySetup = &accessorySetup;

    pal_hap_acc_setup_gen(&platform.keyValueStore, platform.hapPlatform.accessorySetup);

#if IP
    // TCP stream manager.
    HAPPlatformTCPStreamManagerCreate(
            &platform.tcpStreamManager,
            &(const HAPPlatformTCPStreamManagerOptions) {
                    .interfaceName = NULL,        // Listen on all available network interfaces.
                    .port = kHAPNetworkPort_Any,  // Listen on unused port number from the ephemeral port range.
                    .maxConcurrentTCPStreams = PAL_HAP_IP_SESSION_STORAGE_NUM_ELEMENTS });
    platform.hapPlatform.ip.tcpStreamManager = &platform.tcpStreamManager;

    // Service discovery.
    static HAPPlatformServiceDiscovery serviceDiscovery;
    HAPPlatformServiceDiscoveryCreate(
            &serviceDiscovery,
            &(const HAPPlatformServiceDiscoveryOptions) {
                    0 /* Register services on all available network interfaces. */
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

    platform.hapPlatform.authentication.mfiTokenAuth =
            HAPPlatformMFiTokenAuthIsProvisioned(&platform.mfiTokenAuth) ? &platform.mfiTokenAuth : NULL;
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

static const char *help = \
    "usage: %s [options]\n"
    "options:\n"
    "  -d           set the scripts directory\n"
    "  -e           set the entry script name\n"
    "  -h, --help   display this help and exit\n";

static const char *progname = "homekit-bridge";
static const char *scripts_dir = BRIDGE_WORK_DIR;
static const char *entry = BRIDGE_LUA_ENTRY_DEFAULT;

static void usage(const char* message) {
    if (message) {
        if (*message == '-') {
            fprintf(stderr, "%s: unrecognized option '%s'\n", progname, message);
        } else {
            fprintf(stderr, "%s: %s\n", progname, message);
        }
    }
    fprintf(stderr, help, progname);
}

static void doargs(int argc, char *argv[]) {
    if (argv[0] && *argv[0] != 0) {
        progname = argv[0];
    }
    for (int i = 1; i < argc; i++) {
        if (HAPStringAreEqual(argv[i], "-h") || HAPStringAreEqual(argv[i], "--help")) {
            usage(NULL);
            exit(EXIT_SUCCESS);
        } else if (HAPStringAreEqual(argv[i], "-d")) {
            scripts_dir = argv[++i];
            if (!scripts_dir || *scripts_dir == 0 || (*scripts_dir == '-' && scripts_dir[1] != 0)) {
                usage("'-d' needs argument");
                exit(EXIT_FAILURE);
            }
        } else if (HAPStringAreEqual(argv[i], "-e")) {
            entry = argv[++i];
            if (!entry || *entry == 0 || (*entry == '-' && entry[1] != 0)) {
                usage("'-e' needs argument");
                exit(EXIT_FAILURE);
            }
        } else {
            usage(argv[i]);
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[]) {
    HAPAssert(HAPGetCompatibilityVersion() == HAP_COMPATIBILITY_VERSION);

    // Parse arguments.
    doargs(argc, argv);

    // Initialize global platform objects.
    init_platform();

    app_init(&platform.hapPlatform);

    // Run lua entry.
    HAPAssert(app_lua_run(scripts_dir, entry));

    // Run main loop until explicitly stopped.
    HAPPlatformRunLoopRun();
    // Run loop stopped explicitly by calling function HAPPlatformRunLoopStop.

    // Close lua state.
    app_lua_close();

    app_deinit();

    deinit_platform();

    return 0;
}
