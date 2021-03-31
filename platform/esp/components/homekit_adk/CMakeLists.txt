set(TOP_DIR ${COMPONENT_DIR}/../../../..)
include(${TOP_DIR}/cmake/adk.cmake)

set(PORT_SRCS   "port/src/HAPPlatform.c"
                "port/src/HAPPlatformAbort.c"
                "port/src/HAPPlatformAccessorySetup.c"
                "port/src/HAPPlatformAccessorySetupDisplay.c"
                "port/src/HAPPlatformAccessorySetupNFC.c"
                "port/src/HAPPlatformBLEPeripheralManager.c"
                "port/src/HAPPlatformClock.c"
                "port/src/HAPPlatformKeyValueStore.c"
                "port/src/HAPPlatformLog.c"
                "port/src/HAPPlatformMFiHWAuth.c"
                "port/src/HAPPlatformMFiTokenAuth.c"
                "port/src/HAPPlatformRandomNumber.c"
                "port/src/HAPPlatformRunLoop.c"
                "port/src/HAPPlatformServiceDiscovery.c"
                "port/src/HAPPlatformTCPStreamManager.c")

set(PORT_INC_DIR "port/include")

set(SRCS "${ADK_SRCS}" "${PORT_SRCS}")
set(INC_DIR "${ADK_INC_DIR}" "${PORT_INC_DIR}")

idf_component_register(SRCS "${SRCS}"
                INCLUDE_DIRS "${INC_DIR}"
                REQUIRES
                PRIV_REQUIRES nvs_flash mdns)
