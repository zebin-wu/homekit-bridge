# Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
#
# Licensed under the Apache License, Version 2.0 (the “License”);
# you may not use this file except in compliance with the License.
# See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/adk.cmake)

set(PORT_SRCS
    port/src/HAPPlatform.c
    port/src/HAPPlatformAbort.c
    port/src/HAPCrypto.c
    port/src/HAPPlatformAccessorySetup.c
    port/src/HAPPlatformAccessorySetupDisplay.c
    port/src/HAPPlatformAccessorySetupNFC.c
    port/src/HAPPlatformBLEPeripheralManager.c
    port/src/HAPPlatformClock.c
    port/src/HAPPlatformKeyValueStore.c
    port/src/HAPPlatformLog.c
    port/src/HAPPlatformMFiHWAuth.c
    port/src/HAPPlatformMFiTokenAuth.c
    port/src/HAPPlatformRandomNumber.c
    port/src/HAPPlatformRunLoop.c
    port/src/HAPPlatformServiceDiscovery.c
    port/src/HAPPlatformTCPStreamManager.c
)

set(PORT_INC_DIR port/include)

idf_component_register(
    SRCS ${ADK_SRCS} ${PORT_SRCS}
    INCLUDE_DIRS ${ADK_INC_DIRS} ${PORT_INC_DIR}
    REQUIRES
    PRIV_REQUIRES nvs_flash mdns
)

add_definitions(-DHAP_LOG_LEVEL=${CONFIG_HAP_LOG_LEVEL})
