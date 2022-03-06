// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lualib.h>
#include <lauxlib.h>
#include <pal/hap.h>
#include <pal/memory.h>
#include <pal/nvs.h>
#include <HAP.h>
#include <HAPCharacteristic.h>
#include <HAPAccessorySetup.h>

#include "app_int.h"
#include "lc.h"

#define lhap_safe_free(p)     do { if (p) { pal_mem_free((void *)p); (p) = NULL; } } while (0)

#define LHAP_NVS_NAMESPACE "bridge::lhaplib"

/**
 * Default number of services and characteristics contained in the accessory.
 */
#define LHAP_ATTR_CNT_DFT ((size_t) 17)
#define LHAP_CHAR_READ_CNT_DFT ((size_t) 10)
#define LHAP_CHAR_WRITE_CNT_DFT ((size_t) 2)
#define LHAP_CHAR_NOTIFY_CNT_DFT ((size_t) 0)

/**
 * Default maxium number of bridged accessories.
 */
#define LHAP_BRIDGED_ACCS_MAX_CNT_DFT ((size_t) 10)

/**
 * IID constants.
 */
#define kIID_AccessoryInformation                 ((uint64_t) 1)
#define kIID_AccessoryInformationIdentify         ((uint64_t) 2)
#define kIID_AccessoryInformationManufacturer     ((uint64_t) 3)
#define kIID_AccessoryInformationModel            ((uint64_t) 4)
#define kIID_AccessoryInformationName             ((uint64_t) 5)
#define kIID_AccessoryInformationSerialNumber     ((uint64_t) 6)
#define kIID_AccessoryInformationFirmwareRevision ((uint64_t) 7)
#define kIID_AccessoryInformationHardwareRevision ((uint64_t) 8)
#define kIID_AccessoryInformationADKVersion       ((uint64_t) 9)

#define kIID_HAPProtocolInformation                 ((uint64_t) 10)
#define kIID_HAPProtocolInformationServiceSignature ((uint64_t) 11)
#define kIID_HAPProtocolInformationVersion          ((uint64_t) 12)

#define kIID_Pairing                ((uint64_t) 13)
#define kIID_PairingPairSetup       ((uint64_t) 14)
#define kIID_PairingPairVerify      ((uint64_t) 15)
#define kIID_PairingPairingFeatures ((uint64_t) 16)
#define kIID_PairingPairingPairings ((uint64_t) 17)

HAP_STATIC_ASSERT(LHAP_ATTR_CNT_DFT == 9 + 3 + 5, AttributeCount_mismatch);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPBoolCharacteristic accessoryInformationIdentifyCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_AccessoryInformationIdentify,
    .characteristicType = &kHAPCharacteristicType_Identify,
    .debugDescription = kHAPCharacteristicDebugDescription_Identify,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = NULL, .handleWrite = HAPHandleAccessoryInformationIdentifyWrite }
};

const HAPStringCharacteristic accessoryInformationManufacturerCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationManufacturer,
    .characteristicType = &kHAPCharacteristicType_Manufacturer,
    .debugDescription = kHAPCharacteristicDebugDescription_Manufacturer,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationManufacturerRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationModelCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationModel,
    .characteristicType = &kHAPCharacteristicType_Model,
    .debugDescription = kHAPCharacteristicDebugDescription_Model,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationModelRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationNameRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationSerialNumberCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationSerialNumber,
    .characteristicType = &kHAPCharacteristicType_SerialNumber,
    .debugDescription = kHAPCharacteristicDebugDescription_SerialNumber,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationSerialNumberRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationFirmwareRevisionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationFirmwareRevision,
    .characteristicType = &kHAPCharacteristicType_FirmwareRevision,
    .debugDescription = kHAPCharacteristicDebugDescription_FirmwareRevision,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationFirmwareRevisionRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationHardwareRevisionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationHardwareRevision,
    .characteristicType = &kHAPCharacteristicType_HardwareRevision,
    .debugDescription = kHAPCharacteristicDebugDescription_HardwareRevision,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationHardwareRevisionRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationADKVersionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationADKVersion,
    .characteristicType = &kHAPCharacteristicType_ADKVersion,
    .debugDescription = kHAPCharacteristicDebugDescription_ADKVersion,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = true,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationADKVersionRead, .handleWrite = NULL }
};

const HAPService accessoryInformationService = {
    .iid = kIID_AccessoryInformation,
    .serviceType = &kHAPServiceType_AccessoryInformation,
    .debugDescription = kHAPServiceDebugDescription_AccessoryInformation,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &accessoryInformationIdentifyCharacteristic,
                                                            &accessoryInformationManufacturerCharacteristic,
                                                            &accessoryInformationModelCharacteristic,
                                                            &accessoryInformationNameCharacteristic,
                                                            &accessoryInformationSerialNumberCharacteristic,
                                                            &accessoryInformationFirmwareRevisionCharacteristic,
                                                            &accessoryInformationHardwareRevisionCharacteristic,
                                                            &accessoryInformationADKVersionCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPDataCharacteristic hapProtocolInformationServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_HAPProtocolInformationServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

static const HAPStringCharacteristic hapProtocolInformationVersionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_HAPProtocolInformationVersion,
    .characteristicType = &kHAPCharacteristicType_Version,
    .debugDescription = kHAPCharacteristicDebugDescription_Version,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleHAPProtocolInformationVersionRead, .handleWrite = NULL }
};

const HAPService hapProtocolInformationService = {
    .iid = kIID_HAPProtocolInformation,
    .serviceType = &kHAPServiceType_HAPProtocolInformation,
    .debugDescription = kHAPServiceDebugDescription_HAPProtocolInformation,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = true } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &hapProtocolInformationServiceSignatureCharacteristic,
                                                            &hapProtocolInformationVersionCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPTLV8Characteristic pairingPairSetupCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_PairingPairSetup,
    .characteristicType = &kHAPCharacteristicType_PairSetup,
    .debugDescription = kHAPCharacteristicDebugDescription_PairSetup,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = true,
                             .writableWithoutSecurity = true } },
    .callbacks = { .handleRead = HAPHandlePairingPairSetupRead, .handleWrite = HAPHandlePairingPairSetupWrite }
};

static const HAPTLV8Characteristic pairingPairVerifyCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_PairingPairVerify,
    .characteristicType = &kHAPCharacteristicType_PairVerify,
    .debugDescription = kHAPCharacteristicDebugDescription_PairVerify,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = true,
                             .writableWithoutSecurity = true } },
    .callbacks = { .handleRead = HAPHandlePairingPairVerifyRead, .handleWrite = HAPHandlePairingPairVerifyWrite }
};

static const HAPUInt8Characteristic pairingPairingFeaturesCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_PairingPairingFeatures,
    .characteristicType = &kHAPCharacteristicType_PairingFeatures,
    .debugDescription = kHAPCharacteristicDebugDescription_PairingFeatures,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsDisconnectedNotification = false,
                             .supportsBroadcastNotification = false,
                             .readableWithoutSecurity = true,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_None,
    .constraints = { .minimumValue = 0,
                     .maximumValue = UINT8_MAX,
                     .stepValue = 0,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HAPHandlePairingPairingFeaturesRead, .handleWrite = NULL }
};

static const HAPTLV8Characteristic pairingPairingPairingsCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_PairingPairingPairings,
    .characteristicType = &kHAPCharacteristicType_PairingPairings,
    .debugDescription = kHAPCharacteristicDebugDescription_PairingPairings,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandlePairingPairingPairingsRead,
                   .handleWrite = HAPHandlePairingPairingPairingsWrite }
};

const HAPService pairingService = {
    .iid = kIID_Pairing,
    .serviceType = &kHAPServiceType_Pairing,
    .debugDescription = kHAPServiceDebugDescription_Pairing,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &pairingPairSetupCharacteristic,
                                                            &pairingPairVerifyCharacteristic,
                                                            &pairingPairingFeaturesCharacteristic,
                                                            &pairingPairingPairingsCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

#define LHAP_BRIDGED_ACCESSORY_IID_DFT 2

#define LHAP_CASE_CHAR_FORMAT_CODE(format, ptr, code) \
    case kHAPCharacteristicFormat_ ## format: \
        { HAP ## format ## Characteristic *p = ptr; code; } break;

#define LHAP_LOG_TYPE_ERROR(L, name, idx, type) \
    HAPLogError(&lhap_log, "%s: wrong type of %s (%s excepted, got %s)", \
        __func__, name, lua_typename(L, type), luaL_typename(L, idx));

static const HAPLogObject lhap_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "lhap",
};

static const char *lhap_transport_type_strs[] = {
    "",
    "IP",
    "BLE",
    NULL,
};

static const char *lhap_accessory_category_strs[] = {
    "BridgedAccessory",
    "Other",
    "Bridges",
    "Fans",
    "GarageDoorOpeners",
    "Lighting",
    "Locks",
    "Outlets",
    "Switches",
    "Thermostats",
    "Sensors",
    "SecuritySystems",
    "Doors",
    "Windows",
    "WindowCoverings",
    "ProgrammableSwitches",
    "RangeExtenders",
    "IPCameras",
    NULL,
    "AirPurifiers",
    "Heaters",
    "AirConditioners",
    "Humidifiers",
    "Dehumidifiers",
    NULL,
    NULL,
    NULL,
    NULL,
    "Sprinklers",
    "Faucets",
    "ShowerSystems"
};

static const char *lhap_characteristic_format_strs[] = {
    "Data",
    "Bool",
    "UInt8",
    "UInt16",
    "UInt32",
    "UInt64",
    "Int",
    "Float",
    "String",
    "TLV8",
};

static const char *lhap_characteristic_units_strs[] = {
    "None",
    "Celsius",
    "ArcDegrees",
    "Percentage",
    "Lux",
    "Seconds",
};

static const char *lhap_server_state_strs[] = {
    "Idle",
    "Running",
    "Stopping",
};

static const size_t lhap_characteristic_struct_size[] = {
    sizeof(HAPDataCharacteristic),
    sizeof(HAPBoolCharacteristic),
    sizeof(HAPUInt8Characteristic),
    sizeof(HAPUInt16Characteristic),
    sizeof(HAPUInt32Characteristic),
    sizeof(HAPUInt64Characteristic),
    sizeof(HAPIntCharacteristic),
    sizeof(HAPFloatCharacteristic),
    sizeof(HAPStringCharacteristic),
    sizeof(HAPTLV8Characteristic),
};

// Lua light userdata.
typedef struct lhap_lightuserdata {
    const char *name;
    void *ptr;
} lhap_lightuserdata;

static const lhap_lightuserdata lhap_accessory_services_userdatas[] = {
    {"AccessoryInformationService", (void *)&accessoryInformationService},
    {"HAPProtocolInformationService", (void *)&hapProtocolInformationService},
    {"PairingService", (void *)&pairingService},
    {NULL, NULL},
};

typedef struct lhap_service_type {
    const char *name;
    const HAPUUID *type;
    const char *debugDescription;
} lhap_service_type;

#define LHAP_SERVICE_TYPE_FORMAT(type) \
    { #type, &kHAPServiceType_##type, kHAPServiceDebugDescription_##type }

static const lhap_service_type lhap_service_type_tab[] = {
    LHAP_SERVICE_TYPE_FORMAT(AccessoryInformation),
    LHAP_SERVICE_TYPE_FORMAT(GarageDoorOpener),
    LHAP_SERVICE_TYPE_FORMAT(LightBulb),
    LHAP_SERVICE_TYPE_FORMAT(LockManagement),
    LHAP_SERVICE_TYPE_FORMAT(LockMechanism),
    LHAP_SERVICE_TYPE_FORMAT(Outlet),
    LHAP_SERVICE_TYPE_FORMAT(Switch),
    LHAP_SERVICE_TYPE_FORMAT(Thermostat),
    LHAP_SERVICE_TYPE_FORMAT(Pairing),
    LHAP_SERVICE_TYPE_FORMAT(SecuritySystem),
    LHAP_SERVICE_TYPE_FORMAT(CarbonMonoxideSensor),
    LHAP_SERVICE_TYPE_FORMAT(ContactSensor),
    LHAP_SERVICE_TYPE_FORMAT(Door),
    LHAP_SERVICE_TYPE_FORMAT(HumiditySensor),
    LHAP_SERVICE_TYPE_FORMAT(LeakSensor),
    LHAP_SERVICE_TYPE_FORMAT(LightSensor),
    LHAP_SERVICE_TYPE_FORMAT(MotionSensor),
    LHAP_SERVICE_TYPE_FORMAT(OccupancySensor),
    LHAP_SERVICE_TYPE_FORMAT(SmokeSensor),
    LHAP_SERVICE_TYPE_FORMAT(StatelessProgrammableSwitch),
    LHAP_SERVICE_TYPE_FORMAT(TemperatureSensor),
    LHAP_SERVICE_TYPE_FORMAT(Window),
    LHAP_SERVICE_TYPE_FORMAT(WindowCovering),
    LHAP_SERVICE_TYPE_FORMAT(AirQualitySensor),
    LHAP_SERVICE_TYPE_FORMAT(BatteryService),
    LHAP_SERVICE_TYPE_FORMAT(CarbonDioxideSensor),
    LHAP_SERVICE_TYPE_FORMAT(HAPProtocolInformation),
    LHAP_SERVICE_TYPE_FORMAT(Fan),
    LHAP_SERVICE_TYPE_FORMAT(Slat),
    LHAP_SERVICE_TYPE_FORMAT(FilterMaintenance),
    LHAP_SERVICE_TYPE_FORMAT(AirPurifier),
    LHAP_SERVICE_TYPE_FORMAT(HeaterCooler),
    LHAP_SERVICE_TYPE_FORMAT(HumidifierDehumidifier),
    LHAP_SERVICE_TYPE_FORMAT(ServiceLabel),
    LHAP_SERVICE_TYPE_FORMAT(IrrigationSystem),
    LHAP_SERVICE_TYPE_FORMAT(Valve),
    LHAP_SERVICE_TYPE_FORMAT(Faucet),
    LHAP_SERVICE_TYPE_FORMAT(CameraRTPStreamManagement),
    LHAP_SERVICE_TYPE_FORMAT(Microphone),
    LHAP_SERVICE_TYPE_FORMAT(Speaker),
};

typedef struct lhap_characteristic_type {
    const char *name;
    const HAPUUID *type;
    const char *debugDescription;
} lhap_characteristic_type;

#define LHAP_CHARACTERISTIC_TYPE_FORMAT(type) \
{ \
    #type, \
    &kHAPCharacteristicType_##type, \
    kHAPCharacteristicDebugDescription_##type, \
}

static const lhap_characteristic_type lhap_characteristic_type_tab[] = {
    LHAP_CHARACTERISTIC_TYPE_FORMAT(AdministratorOnlyAccess),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(AudioFeedback),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Brightness),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CoolingThresholdTemperature),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentDoorState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentHeatingCoolingState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentRelativeHumidity),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentTemperature),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(HeatingThresholdTemperature),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Hue),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Identify),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LockControlPoint),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LockManagementAutoSecurityTimeout),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LockLastKnownAction),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LockCurrentState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LockTargetState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Logs),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Manufacturer),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Model),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(MotionDetected),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Name),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ObstructionDetected),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(On),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(OutletInUse),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(RotationDirection),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(RotationSpeed),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Saturation),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SerialNumber),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetDoorState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetHeatingCoolingState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetRelativeHumidity),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetTemperature),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TemperatureDisplayUnits),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Version),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PairSetup),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PairVerify),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PairingFeatures),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PairingPairings),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(FirmwareRevision),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(HardwareRevision),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(AirParticulateDensity),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(AirParticulateSize),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SecuritySystemCurrentState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SecuritySystemTargetState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(BatteryLevel),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CarbonMonoxideDetected),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ContactSensorState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentAmbientLightLevel),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentHorizontalTiltAngle),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentPosition),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentVerticalTiltAngle),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(HoldPosition),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LeakDetected),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(OccupancyDetected),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PositionState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ProgrammableSwitchEvent),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(StatusActive),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SmokeDetected),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(StatusFault),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(StatusJammed),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(StatusLowBattery),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(StatusTampered),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetHorizontalTiltAngle),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetPosition),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetVerticalTiltAngle),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SecuritySystemAlarmType),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ChargingState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CarbonMonoxideLevel),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CarbonMonoxidePeakLevel),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CarbonDioxideDetected),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CarbonDioxideLevel),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CarbonDioxidePeakLevel),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(AirQuality),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ServiceSignature),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(AccessoryFlags),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LockPhysicalControls),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetAirPurifierState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentAirPurifierState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentSlatState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(FilterLifeLevel),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(FilterChangeIndication),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ResetFilterIndication),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentFanState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Active),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentHeaterCoolerState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetHeaterCoolerState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentHumidifierDehumidifierState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetHumidifierDehumidifierState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(WaterLevel),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SwingMode),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetFanState),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SlatType),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentTiltAngle),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetTiltAngle),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(OzoneDensity),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(NitrogenDioxideDensity),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SulphurDioxideDensity),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PM2_5Density),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PM10Density),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(VOCDensity),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(RelativeHumidityDehumidifierThreshold),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(RelativeHumidityHumidifierThreshold),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ServiceLabelIndex),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ServiceLabelNamespace),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ColorTemperature),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ProgramMode),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(InUse),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SetDuration),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(RemainingDuration),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ValveType),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(IsConfigured),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ActiveIdentifier),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ADKVersion),
};

#if LUA_MAXINTEGER < UINT32_MAX
#define LHAP_UINT32_MAX LUA_MAXINTEGER
#else
#define LHAP_UINT32_MAX UINT32_MAX
#endif

static const struct lhap_char_integer_val_range {
    lua_Integer min;
    lua_Integer max;
} lhap_char_integer_value_range_tab[] = {
    [kHAPCharacteristicFormat_UInt8] = {0, UINT8_MAX},
    [kHAPCharacteristicFormat_UInt16] = {0, UINT16_MAX},
    [kHAPCharacteristicFormat_UInt32] = {0, LHAP_UINT32_MAX},
    [kHAPCharacteristicFormat_UInt64] = {0, LUA_MAXINTEGER},
    [kHAPCharacteristicFormat_Int] = {INT32_MIN, INT32_MAX},
};

typedef struct lhap_desc {
    bool inited:1;
    bool is_started:1;

    HAPAccessory *primary_acc;
    HAPAccessory **bridged_accs;
    size_t bridged_accs_max;
    size_t bridged_accs_cnt;

    HAPPlatform *platform;
    HAPAccessoryServerRef server;
    HAPAccessoryServerOptions server_options;
    HAPAccessoryServerCallbacks server_cbs;
} lhap_desc;

static lhap_desc gv_lhap_desc = {
    .server_options = {
        .maxPairings = kHAPPairingStorage_MinElements
    }
};

static void lhap_rawsetp_reset(lua_State *L, int idx, const void *p) {
    lua_pushnil(L);
    lua_rawsetp(L, idx, p);
}

// Return a new copy from the str on the "idx" of the stack.
static char *lhap_new_str(lua_State *L, int idx) {
    size_t len;
    const char *str = lua_tolstring(L, idx, &len);
    if (str[len] != '\0') {
        HAPLogError(&lhap_log, "%s: Invalid string.", __func__);
        return NULL;
    }
    char *copy = pal_mem_alloc(len + 1);
    if (!copy) {
        HAPLogError(&lhap_log, "%s: Failed to alloc.", __func__);
        return NULL;
    }
    HAPRawBufferCopyBytes(copy, str, len);
    copy[len] = '\0';
    return copy;
}

// Find the string and return the string index, if not found, return -1.
static int lhap_lookup_by_name(const char *name, const char *strs[], int len) {
    for (int i = 0; i < len; i++) {
        if (strs[i] && HAPStringAreEqual(name, strs[i])) {
            return i;
        }
    }
    return -1;
}

// Check if the service is one of "lhap_accessory_services_userdatas"
static bool lhap_service_is_light_userdata(HAPService *service) {
    for (const lhap_lightuserdata *ud = lhap_accessory_services_userdatas;
        ud->ptr; ud++) {
        if (service == ud->ptr) {
            return true;
        }
    }
    return false;
}

static lua_Integer
lhap_tointegerx(lua_State *L, int idx, int *isnum, HAPCharacteristicFormat format, bool is_unsigned) {
    lua_Integer num;
    num = lua_tointegerx(L, idx, isnum);
    if (!(*isnum)) {
        return num;
    }
    lua_Integer min = is_unsigned ? 0 : lhap_char_integer_value_range_tab[format].min;
    if (num < min || num > lhap_char_integer_value_range_tab[format].max) {
        HAPLogError(&lhap_log, "%s: Integer is out of range("
            LUA_INTEGER_FMT ", " LUA_INTEGER_FMT ").",
            __func__, lhap_char_integer_value_range_tab[format].min,
            lhap_char_integer_value_range_tab[format].max);
        *isnum = false;
        return 0;
    }
    return num;
}

static bool
lhap_accessory_aid_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    int isnum;
    lua_Integer aid = lua_tointegerx(L, -1, &isnum);
    if (!isnum || aid <= 0) {
        return false;
    }
    ((HAPAccessory *)arg)->aid = aid;
    return true;
}

static bool
lhap_accessory_category_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    int idx = lhap_lookup_by_name(lua_tostring(L, -1),
        lhap_accessory_category_strs,
        HAPArrayCount(lhap_accessory_category_strs));
    if (idx == -1) {
        return false;
    }
    ((HAPAccessory *)arg)->category = idx;
    return true;
}

static bool
lhap_accessory_name_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPAccessory *accessory = arg;

    return (*((char **)&accessory->name) =
        lhap_new_str(L, -1)) ? true : false;
}

static bool
lhap_accessory_mfg_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPAccessory *accessory = arg;

    return (*((char **)&accessory->manufacturer) =
        lhap_new_str(L, -1)) ? true : false;
}

static bool
lhap_accessory_model_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPAccessory *accessory = arg;

    return (*((char **)&accessory->model) =
        lhap_new_str(L, -1)) ? true : false;
}

static bool
lhap_accessory_sn_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPAccessory *accessory = arg;

    return (*((char **)&accessory->serialNumber) =
        lhap_new_str(L, -1)) ? true : false;
}

static bool
lhap_accessory_fw_ver_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPAccessory *accessory = arg;

    return (*((char **)&accessory->firmwareVersion) =
        lhap_new_str(L, -1)) ? true : false;
}

static bool
lhap_accessory_hw_ver_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPAccessory *accessory = arg;

    return (*((char **)&accessory->hardwareVersion) =
        lhap_new_str(L, -1)) ? true : false;
}

static bool
lhap_service_iid_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    int isnum;
    lua_Integer iid = lua_tointegerx(L, -1, &isnum);
    if (!isnum || iid <= (lua_Integer)LHAP_ATTR_CNT_DFT) {
        HAPLogError(&lhap_log, "%s: Invalid IID.", __func__);
        return false;
    }
    ((HAPService *)arg)->iid = iid;
    return true;
}

static bool
lhap_service_type_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPService *service = arg;
    const char *str = lua_tostring(L, -1);
    for (int i = 0; i < HAPArrayCount(lhap_service_type_tab);
        i++) {
        if (HAPStringAreEqual(str, lhap_service_type_tab[i].name)) {
            service->serviceType = lhap_service_type_tab[i].type;
            service->debugDescription = lhap_service_type_tab[i].debugDescription;
            return true;
        }
    }
    return false;
}

static bool
lhap_service_props_primary_service_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPServiceProperties *)arg)->primaryService = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_service_props_hidden_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPServiceProperties *)arg)->hidden = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_service_props_supports_conf_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPServiceProperties *)arg)->ble.supportsConfiguration = lua_toboolean(L, -1);
    return true;
}

static const lc_table_kv lhap_service_props_ble_kvs[] = {
    {"supportsConfiguration", LC_TBOOLEAN, lhap_service_props_supports_conf_cb},
    {NULL, LUA_TNONE, NULL},
};

static bool
lhap_service_props_ble_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    return lc_traverse_table(L, -1, lhap_service_props_ble_kvs, arg);
}

static const lc_table_kv lhap_service_props_kvs[] = {
    {"primaryService", LC_TBOOLEAN, lhap_service_props_primary_service_cb},
    {"hidden", LC_TBOOLEAN, lhap_service_props_hidden_cb},
    {"ble", LC_TTABLE, lhap_service_props_ble_cb},
    {NULL, LC_TNONE, NULL},
};

static bool
lhap_service_props_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPService *service = arg;

    return lc_traverse_table(L, -1, lhap_service_props_kvs, &service->properties);
}

bool lhap_service_linked_services_arr_cb(lua_State *L, size_t i, void *arg) {
    uint16_t *iids = arg;

    if (!lua_isnumber(L, -1)) {
        char name[32];
        snprintf(name, sizeof(name), "linkedServices[%zu]", i + 1);
        LHAP_LOG_TYPE_ERROR(L, name, -1, LUA_TNUMBER);
        return false;
    }

    int isnum;
    iids[i] = lhap_tointegerx(L, -1, &isnum, kHAPCharacteristicFormat_UInt16, true);
    if (!isnum) {
        return false;
    }
    return true;
}

static bool
lhap_service_linked_services_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPService *service = arg;

    uint16_t **piids = (uint16_t **)&(service->linkedServices);
    lua_Unsigned len = lua_rawlen(L, -1);
    if (!len) {
        HAPLogError(&lhap_log, "%s: Invalid array.", __func__);
        return false;
    }
    uint16_t *iids = pal_mem_alloc(sizeof(uint16_t) * (len + 1));
    if (!iids) {
        HAPLogError(&lhap_log, "%s: Failed to alloc.", __func__);
        return false;
    }
    iids[len] = 0;
    if (!lc_traverse_array(L, -1, lhap_service_linked_services_arr_cb,
        iids)) {
        HAPLogError(&lhap_log, "%s: Failed to parse linkedServices.", __func__);
        return false;
    }
    *piids = iids;
    return true;
}

static bool
lhap_char_iid_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    int isnum;
    lua_Integer iid = lua_tointegerx(L, -1, &isnum);
    if (!isnum || iid <= (lua_Integer)LHAP_ATTR_CNT_DFT) {
        HAPLogError(&lhap_log, "%s: Invalid IID.", __func__);
        return false;
    }
    ((HAPBaseCharacteristic *)arg)->iid = iid;
    return true;
}

static bool
lhap_char_type_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPBaseCharacteristic *c = arg;
    const char *str = lua_tostring(L, -1);
    for (int i = 0; i < HAPArrayCount(lhap_characteristic_type_tab);
        i++) {
        if (HAPStringAreEqual(str, lhap_characteristic_type_tab[i].name)) {
            c->characteristicType = lhap_characteristic_type_tab[i].type;
            c->debugDescription = lhap_characteristic_type_tab[i].debugDescription;
            return true;
        }
    }
    HAPLogError(&lhap_log, "%s: error type.", __func__);
    return false;
}

static bool
lhap_char_mfg_desc_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPBaseCharacteristic *ch = arg;

    return (*((char **)&ch->manufacturerDescription) = lhap_new_str(L, -1)) ? true : false;
}

static bool
lhap_char_props_readable_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->readable = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_writable_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->writable = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_support_evt_notify_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->supportsEventNotification = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_hidden_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->hidden = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_read_req_admin_pms_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->readRequiresAdminPermissions = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_write_req_admin_pms_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->writeRequiresAdminPermissions = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_req_timed_write_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->requiresTimedWrite = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_support_auth_data_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->supportsAuthorizationData = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_ip_control_point_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->ip.controlPoint = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_ip_support_write_resp_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->ip.supportsWriteResponse = lua_toboolean(L, -1);
    return true;
}

static const lc_table_kv lhap_char_props_ip_kvs[] = {
    {"controlPoint", LC_TBOOLEAN, lhap_char_props_ip_control_point_cb},
    {"supportsWriteResponse", LC_TBOOLEAN, lhap_char_props_ip_support_write_resp_cb},
    {NULL, LC_TNONE, NULL},
};

static bool
lhap_char_props_ip_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    return lc_traverse_table(L, -1, lhap_char_props_ip_kvs, arg);
}

static bool
lhap_char_props_ble_support_bc_notify_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->ble.supportsBroadcastNotification =
        lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_ble_support_disconn_notify_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->ble.supportsDisconnectedNotification =
        lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_ble_read_without_sec_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->ble.readableWithoutSecurity =
        lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_ble_write_without_sec_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    ((HAPCharacteristicProperties *)arg)->ble.writableWithoutSecurity =
        lua_toboolean(L, -1);
    return true;
}

static const lc_table_kv lhap_char_props_ble_kvs[] = {
    {
        "supportsBroadcastNotification",
        LC_TBOOLEAN,
        lhap_char_props_ble_support_bc_notify_cb
    },
    {
        "supportsDisconnectedNotification",
        LC_TBOOLEAN,
        lhap_char_props_ble_support_disconn_notify_cb
    },
    {
        "readableWithoutSecurity",
        LC_TBOOLEAN,
        lhap_char_props_ble_read_without_sec_cb
    },
    {
        "writableWithoutSecurity",
        LC_TBOOLEAN,
        lhap_char_props_ble_write_without_sec_cb
    },
    {NULL, LC_TNONE, NULL},
};

static bool
lhap_char_props_ble_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    return lc_traverse_table(L, -1, lhap_char_props_ble_kvs, arg);
}

static const lc_table_kv lhap_char_props_kvs[] = {
    {"readable", LC_TBOOLEAN, lhap_char_props_readable_cb},
    {"writable", LC_TBOOLEAN, lhap_char_props_writable_cb},
    {"supportsEventNotification", LC_TBOOLEAN, lhap_char_props_support_evt_notify_cb},
    {"hidden", LC_TBOOLEAN, lhap_char_props_hidden_cb},
    {"readRequiresAdminPermissions", LC_TBOOLEAN, lhap_char_props_read_req_admin_pms_cb},
    {"writeRequiresAdminPermissions", LC_TBOOLEAN, lhap_char_props_write_req_admin_pms_cb},
    {"requiresTimedWrite", LC_TBOOLEAN, lhap_char_props_req_timed_write_cb},
    {"supportsAuthorizationData", LC_TBOOLEAN, lhap_char_props_support_auth_data_cb},
    {"ip", LC_TTABLE, lhap_char_props_ip_cb},
    {"ble", LC_TTABLE, lhap_char_props_ble_cb},
    {NULL, LC_TNONE, NULL},
};

static bool
lhap_char_props_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPBaseCharacteristic *ch = arg;

    return lc_traverse_table(L, -1, lhap_char_props_kvs, &ch->properties);
}

static bool
lhap_char_units_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)arg)->format;
    if (format < kHAPCharacteristicFormat_UInt8 ||
        format > kHAPCharacteristicFormat_Float) {
        HAPLogError(&lhap_log, "%s: %s characteristic has no unit.",
            __func__, lhap_characteristic_format_strs[format]);
        return false;
    }
    const char *str = lua_tostring(L, -1);
    int idx = lhap_lookup_by_name(str, lhap_characteristic_units_strs,
        HAPArrayCount(lhap_characteristic_units_strs));
    if (idx == -1) {
        HAPLogError(&lhap_log, "%s: Failed to find the unit \"%s\""
            " in lhap_characteristic_units_strs", __func__, str);
        return false;
    }

    switch (format) {
    LHAP_CASE_CHAR_FORMAT_CODE(UInt8, arg, p->units = idx)
    LHAP_CASE_CHAR_FORMAT_CODE(UInt16, arg, p->units = idx)
    LHAP_CASE_CHAR_FORMAT_CODE(UInt32, arg, p->units = idx)
    LHAP_CASE_CHAR_FORMAT_CODE(UInt64, arg, p->units = idx)
    LHAP_CASE_CHAR_FORMAT_CODE(Int, arg, p->units = idx)
    LHAP_CASE_CHAR_FORMAT_CODE(Float, arg, p->units = idx)
    default:
        HAPAssertionFailure();
        break;
    }
    return true;
}

static bool
lhap_char_constraints_max_len_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    int isnum;
    lua_Integer num;
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)arg)->format;
    switch (format) {
    LHAP_CASE_CHAR_FORMAT_CODE(String, arg,
        num = lua_tointegerx(L, -1, &isnum);
        p->constraints.maxLength = num;
    )
    LHAP_CASE_CHAR_FORMAT_CODE(Data, arg,
        num = lua_tointegerx(L, -1, &isnum);
        p->constraints.maxLength = num;
    )
    default:
        HAPLogError(&lhap_log, "%s: The constraints of the %s "
            "characteristic has no maxLength.",
            __func__, lhap_characteristic_format_strs[format]);
        return false;
    }
    if (!isnum) {
        HAPLogError(&lhap_log, "%s: Invalid maxLength", __func__);
        return false;
    }
    if (num < 0 || num > LHAP_UINT32_MAX) {
        HAPLogError(&lhap_log, "%s: maxLength is out of range(0, %u).",
            __func__, LHAP_UINT32_MAX);
        return false;
    }
    return true;
}

#define LHAP_CHAR_CONSTRAINTS_VAL_CB_CODE(member, is_unsigned) \
    int isnum; \
    lua_Integer num = 0; \
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)arg)->format; \
    if (format >= kHAPCharacteristicFormat_UInt8 && \
        format <= kHAPCharacteristicFormat_Int) { \
        num = lhap_tointegerx(L, -1, &isnum, format, is_unsigned); \
    } \
    switch (format) { \
    LHAP_CASE_CHAR_FORMAT_CODE(UInt8, arg, p->constraints.member = num) \
    LHAP_CASE_CHAR_FORMAT_CODE(UInt16, arg, p->constraints.member = num) \
    LHAP_CASE_CHAR_FORMAT_CODE(UInt32, arg, p->constraints.member = num) \
    LHAP_CASE_CHAR_FORMAT_CODE(UInt64, arg, p->constraints.member = num) \
    LHAP_CASE_CHAR_FORMAT_CODE(Int, arg, p->constraints.member = num) \
    LHAP_CASE_CHAR_FORMAT_CODE(Float, arg, \
        p->constraints.member = lua_tonumberx(L, -1, &isnum)) \
    default: \
        HAPLogError(&lhap_log, "%s: The constraints of the %s " \
            "characteristic has no %s.", __func__, \
            lhap_characteristic_format_strs[format], #member); \
        return false; \
    } \
    if (!isnum) { \
        HAPLogError(&lhap_log, "%s: Invalid %s", __func__, #member); \
        return false; \
    } \
    return true;

static bool
lhap_char_constraints_min_val_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    LHAP_CHAR_CONSTRAINTS_VAL_CB_CODE(minimumValue, false)
}

static bool
lhap_char_constraints_max_val_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    LHAP_CHAR_CONSTRAINTS_VAL_CB_CODE(maximumValue, false)
}

static bool
lhap_char_constraints_step_val_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    LHAP_CHAR_CONSTRAINTS_VAL_CB_CODE(stepValue, true)
}

#undef LHAP_CHAR_CONSTRAINTS_VAL_CB_CODE

static bool
lhap_char_constraints_valid_vals_arr_cb(lua_State *L, size_t i, void *arg) {
    uint8_t **vals = arg;

    if (!lua_isnumber(L, -1)) {
        char name[32];
        snprintf(name, sizeof(name), "validValues[%zu]", i + 1);
        LHAP_LOG_TYPE_ERROR(L, name, -1, LUA_TNUMBER);
        return false;
    }

    int isnum;
    *(vals[i]) = lhap_tointegerx(L, -1, &isnum, kHAPCharacteristicFormat_UInt8, true);
    if (!isnum) {
        return false;
    }
    return true;
}

static bool
lhap_char_constraints_valid_vals_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)arg)->format;
    switch (format) {
    LHAP_CASE_CHAR_FORMAT_CODE(UInt8, arg,
        uint8_t ***pValidValues = (uint8_t ***)&(p->constraints.validValues);
        lua_Unsigned len = lua_rawlen(L, -1);
        if (!len) {
            HAPLogError(&lhap_log, "%s: Invalid array.", __func__);
            return false;
        }
        size_t vals_len = (len + 1) * sizeof(uint8_t *);
        uint8_t **vals = pal_mem_alloc(vals_len + sizeof(uint8_t) * len);
        if (!vals) {
            HAPLogError(&lhap_log, "%s: Failed to alloc.", __func__);
            return false;
        }
        for (int i = 0; i < len; i++) {
            vals[i] = (uint8_t *)((char *)vals + vals_len) + i;
        }
        vals[len] = NULL;
        if (!lc_traverse_array(L, -1, lhap_char_constraints_valid_vals_arr_cb,
            vals)) {
            HAPLogError(&lhap_log, "%s: Failed to parse validValues.", __func__);
            return false;
        }
        *pValidValues = vals;
    )
    default:
        HAPLogError(&lhap_log, "%s: The constraints of the %s "
            "characteristic has no validValues.",
            __func__, lhap_characteristic_format_strs[format]);
        return false;
    }
    return true;
}

static bool
lhap_char_constraints_valid_val_range_start_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    int isnum;
    ((HAPUInt8CharacteristicValidValuesRange *)arg)->start = lhap_tointegerx(L, -1,
        &isnum, kHAPCharacteristicFormat_UInt8, true);
    return isnum;
}

static bool
lhap_char_constraints_valid_val_range_end_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    int isnum;
    ((HAPUInt8CharacteristicValidValuesRange *)arg)->end = lhap_tointegerx(L, -1,
        &isnum, kHAPCharacteristicFormat_UInt8, true);
    return isnum;
}

static const lc_table_kv lhap_char_constraints_valid_val_range_kvs[] = {
    {"start", LC_TNUMBER, lhap_char_constraints_valid_val_range_start_cb},
    {"stop", LC_TNUMBER, lhap_char_constraints_valid_val_range_end_cb},
    {NULL, LC_TNONE, NULL},
};

static bool
lhap_char_constraints_valid_vals_ranges_arr_cb(lua_State *L, size_t i, void *arg) {
    HAPUInt8CharacteristicValidValuesRange **ranges = arg;

    if (!lua_istable(L, -1)) {
        char name[32];
        snprintf(name, sizeof(name), "validValuesRanges[%zu]", i + 1);
        LHAP_LOG_TYPE_ERROR(L, name, -1, LUA_TTABLE);
        return false;
    }
    if (!lc_traverse_table(L, -1, lhap_char_constraints_valid_val_range_kvs, ranges[i])) {
        HAPLogError(&lhap_log, "%s: Failed to parse valid value range.", __func__);
        return false;
    }
    return true;
}

static bool
lhap_char_constraints_valid_vals_ranges_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)arg)->format;
    switch (format) {
    LHAP_CASE_CHAR_FORMAT_CODE(UInt8, arg,
        HAPUInt8CharacteristicValidValuesRange ***pValidValuesRanges =
            (HAPUInt8CharacteristicValidValuesRange ***)
            &(p->constraints.validValuesRanges);
        lua_Unsigned len = lua_rawlen(L, -1);
        if (!len) {
            HAPLogError(&lhap_log, "%s: Invalid array.", __func__);
            return false;
        }
        size_t ranges_len = (len + 1) * sizeof(HAPUInt8CharacteristicValidValuesRange *);
        HAPUInt8CharacteristicValidValuesRange **ranges =
            pal_mem_alloc(ranges_len + len * sizeof(HAPUInt8CharacteristicValidValuesRange));
        if (!ranges) {
            HAPLogError(&lhap_log, "%s: Failed to alloc ranges.", __func__);
            return false;
        }
        for (int i = 0; i < len; i++) {
            ranges[i] = (HAPUInt8CharacteristicValidValuesRange *)((char *)ranges + ranges_len) + i;
        }
        ranges[len] = NULL;
        if (!lc_traverse_array(L, -1, lhap_char_constraints_valid_vals_ranges_arr_cb,
            ranges)) {
            HAPLogError(&lhap_log, "%s: Failed to parse validValues.", __func__);
            return false;
        }
        *pValidValuesRanges = ranges;
    )
    default:
        HAPLogError(&lhap_log, "%s: The constraints of the %s "
            "characteristic has no validValues.",
            __func__, lhap_characteristic_format_strs[format]);
        return false;
    }
    return true;
}

static const lc_table_kv lhap_char_constraints_kvs[] = {
    {"maxLen", LC_TNUMBER, lhap_char_constraints_max_len_cb},
    {"minVal", LC_TNUMBER, lhap_char_constraints_min_val_cb},
    {"maxVal", LC_TNUMBER, lhap_char_constraints_max_val_cb},
    {"stepVal", LC_TNUMBER, lhap_char_constraints_step_val_cb},
    {"validVals", LC_TTABLE, lhap_char_constraints_valid_vals_cb},
    {"validValsRanges", LC_TTABLE, lhap_char_constraints_valid_vals_ranges_cb},
    {NULL, LC_TNONE, NULL},
};

static bool
lhap_char_constraints_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    return lc_traverse_table(L, -1, lhap_char_constraints_kvs, arg);
}

static void
lhap_create_request_table(
        lua_State *L,
        HAPTransportType transportType,
        HAPSessionRef *session,
        const bool *const remote,
        const HAPAccessory *accessory,
        const HAPService *service,
        const HAPBaseCharacteristic *characteristic) {
    lua_createtable(L, 0, 2 + (remote ? 1 : 0) + (service ? 1 : 0) + (characteristic ? 1 : 0));
    lua_pushstring(L, lhap_transport_type_strs[transportType]);
    lua_setfield(L, -2, "transportType");
    if (remote) {
        lua_pushboolean(L, *remote);
        lua_setfield(L, -2, "remote");
    }
    lua_pushlightuserdata(L, session);
    lua_setfield(L, -2, "session");
    lua_pushinteger(L, accessory->aid);
    lua_setfield(L, -2, "aid");
    if (service) {
        lua_pushinteger(L, service->iid);
        lua_setfield(L, -2, "sid");
    }
    if (characteristic) {
        lua_pushinteger(L, characteristic->iid);
        lua_setfield(L, -2, "cid");
    }
}

typedef struct lhap_call_context {
    bool in_progress;
    HAPTransportType transportType;
    HAPAccessoryServerRef *server;
    HAPSessionRef *session;
    const HAPAccessory *accessory;
    const HAPService *service;
    const HAPCharacteristic *characteristic;
} lhap_call_context;

union lhap_char_value {
    bool boolean;
    lua_Integer integer;
    lua_Number number;
    struct {
        const char *data;
        size_t len;
    } str;
};

static bool lhap_char_value_is_valid(lua_State *L, int idx, HAPCharacteristicFormat format) {
    bool is_valid = false;
    switch (format) {
    case kHAPCharacteristicFormat_Bool:
        is_valid = lua_isboolean(L, idx);
        break;
    case kHAPCharacteristicFormat_UInt8:
    case kHAPCharacteristicFormat_UInt16:
    case kHAPCharacteristicFormat_UInt32:
    case kHAPCharacteristicFormat_UInt64:
    case kHAPCharacteristicFormat_Int:
    case kHAPCharacteristicFormat_Float:
        is_valid = lua_isnumber(L, idx);
        break;
    case kHAPCharacteristicFormat_Data:
    case kHAPCharacteristicFormat_String:
    case kHAPCharacteristicFormat_TLV8:
        is_valid = lua_isstring(L, idx);
        break;
    }
    return is_valid;
}

static int
lhap_char_value_get(lua_State *L, int idx, HAPCharacteristicFormat format, union lhap_char_value *value) {
    int valid = true;
    if (format >= kHAPCharacteristicFormat_UInt8 && format <= kHAPCharacteristicFormat_Int) {
        value->integer = lhap_tointegerx(L, idx, &valid, format, false);
    } else {
        switch (format) {
        case kHAPCharacteristicFormat_Bool:
            value->boolean = lua_toboolean(L, idx);
            break;
        case kHAPCharacteristicFormat_Float:
            value->number = lua_tonumberx(L, idx, &valid);
            break;
        case kHAPCharacteristicFormat_Data:
        case kHAPCharacteristicFormat_String:
        case kHAPCharacteristicFormat_TLV8:
            value->str.data = lua_tolstring(L, idx, &value->str.len);
            break;
        default:
            HAPAssertionFailure();
        }
    }
    return valid;
}

int lhap_char_handle_read_finsh(lua_State *L, int status, lua_KContext _ctx) {
    lhap_call_context *ctx = (lhap_call_context *)_ctx;
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)ctx->characteristic)->format;
    HAPError err = kHAPError_None;
    if (status != LUA_OK && status != LUA_YIELD) {
        HAPLogError(&lhap_log, "%s: %s", __func__, lua_tostring(L, -1));
        err = kHAPError_Unknown;
    } else if (!lhap_char_value_is_valid(L, -1, format)) {
        err = kHAPError_InvalidData;
    }
    if (ctx->in_progress == false) {
        lua_pushinteger(L, err);
        return 2;
    }
    union lhap_char_value val;
    if (err == kHAPError_None) {
        int valid = lhap_char_value_get(L, -1, format, &val);
        if (!valid) {
            err = kHAPError_InvalidData;
        }
    }
    switch (format) {
    case kHAPCharacteristicFormat_Bool:
        err = HAPBoolCharacteristicResponseReadRequest(ctx->server, ctx->transportType,
            ctx->session, ctx->accessory, ctx->service, ctx->characteristic, err,
            err == kHAPError_None ? val.boolean : false);
        break;
    case kHAPCharacteristicFormat_UInt8:
        err = HAPUInt8CharacteristicResponseReadRequest(ctx->server, ctx->transportType,
            ctx->session, ctx->accessory, ctx->service, ctx->characteristic, err,
            err == kHAPError_None ? val.integer : 0);
        break;
    case kHAPCharacteristicFormat_UInt16:
        err = HAPUInt16CharacteristicResponseReadRequest(ctx->server, ctx->transportType,
            ctx->session, ctx->accessory, ctx->service, ctx->characteristic, err,
            err == kHAPError_None ? val.integer : 0);
        break;
    case kHAPCharacteristicFormat_UInt32:
        err = HAPUInt32CharacteristicResponseReadRequest(ctx->server, ctx->transportType,
            ctx->session, ctx->accessory, ctx->service, ctx->characteristic, err,
            err == kHAPError_None ? val.integer : 0);
        break;
    case kHAPCharacteristicFormat_UInt64:
        err = HAPUInt64CharacteristicResponseReadRequest(ctx->server, ctx->transportType,
            ctx->session, ctx->accessory, ctx->service, ctx->characteristic, err,
            err == kHAPError_None ? val.integer : 0);
        break;
    case kHAPCharacteristicFormat_Int:
        err = HAPIntCharacteristicResponseReadRequest(ctx->server, ctx->transportType,
            ctx->session, ctx->accessory, ctx->service, ctx->characteristic, err,
            err == kHAPError_None ? val.integer : 0);
        break;
    case kHAPCharacteristicFormat_Float:
        err = HAPFloatCharacteristicResponseReadRequest(ctx->server, ctx->transportType,
            ctx->session, ctx->accessory, ctx->service, ctx->characteristic, err,
            err == kHAPError_None ? val.number : 0);
        break;
    case kHAPCharacteristicFormat_Data: {
        err = HAPDataCharacteristicResponseReadRequest(ctx->server, ctx->transportType,
            ctx->session, ctx->accessory, ctx->service, ctx->characteristic, err,
            err == kHAPError_None ? val.str.data : NULL, err == kHAPError_None ? val.str.len : 0);
    } break;
    case kHAPCharacteristicFormat_String:
        err = HAPStringCharacteristicResponseReadRequest(ctx->server, ctx->transportType,
            ctx->session, ctx->accessory, ctx->service, ctx->characteristic, err,
            err == kHAPError_None ? val.str.data : NULL);
        break;
    case kHAPCharacteristicFormat_TLV8:
        // TODO(Zebin Wu): Implement TLV8 in lua.
        HAPAssertionFailure();
    }
    if (err != kHAPError_None) {
        HAPLogError(&lhap_log, "%s: Failed to response read request, error code: %d.", __func__, err);
    }
    return 0;
}

static int lhap_char_handle_read(lua_State *L) {
    // stack: <call_ctx, traceback, func, request>
    lua_KContext call_ctx = (lua_KContext)lua_touserdata(L, 1);
    int status = lua_pcallk(L, 1, 1, 2, call_ctx, lhap_char_handle_read_finsh);
    return lhap_char_handle_read_finsh(L, status, call_ctx);
}

static int lhap_char_handle_read_pcall(lua_State *L) {
    HAPTransportType transportType = lua_tointeger(L, 1);
    HAPAccessoryServerRef *server = lua_touserdata(L, 2);
    HAPSessionRef *session = lua_touserdata(L, 3);
    const HAPAccessory *accessory = lua_touserdata(L, 4);
    const HAPService *service = lua_touserdata(L, 5);
    const HAPBaseCharacteristic *characteristic = lua_touserdata(L, 6);
    const void *pfunc = lua_touserdata(L, 7);
    lua_pop(L, 7);

    lua_State *co = lua_newthread(L);
    lua_pushcfunction(co, lhap_char_handle_read);
    lhap_call_context *call_ctx = lua_newuserdata(co, sizeof(*call_ctx));
    call_ctx->in_progress = false;
    call_ctx->transportType = transportType;
    call_ctx->server = server;
    call_ctx->session = session;
    call_ctx->accessory = accessory;
    call_ctx->service = service;
    call_ctx->characteristic = characteristic;

    lc_pushtraceback(co);

    // push the function
    HAPAssert(lua_rawgetp(co, LUA_REGISTRYINDEX, pfunc) == LUA_TFUNCTION);

    // push the table request
    lhap_create_request_table(co, transportType, session, NULL,
        accessory, service, characteristic);

    int status, nres;
    status = lc_resume(co, L, 4, &nres);
    switch (status) {
    case LUA_OK:
        HAPAssert(nres == 2);
        return 2;
    case LUA_YIELD:
        call_ctx->in_progress = true;
        lua_pushinteger(L, kHAPError_InProgress);
        return 1;
    default:
        return lua_error(L);
    }
}

static HAP_RESULT_USE_CHECK
HAPError lhap_char_base_handleRead(
        lua_State *L,
        HAPAccessoryServerRef *server,
        HAPTransportType transportType,
        HAPSessionRef *session,
        const HAPAccessory *accessory,
        const HAPService *service,
        const HAPBaseCharacteristic *characteristic,
        const void *pfunc) {
    HAPAssert(lua_gettop(L) == 0);

    lua_pushcfunction(L, lhap_char_handle_read_pcall);
    lua_pushinteger(L, transportType);
    lua_pushlightuserdata(L, server);
    lua_pushlightuserdata(L, session);
    lua_pushlightuserdata(L, (void *)accessory);
    lua_pushlightuserdata(L, (void *)service);
    lua_pushlightuserdata(L, (void *)characteristic);
    lua_pushlightuserdata(L, (void *)pfunc);
    int status = lua_pcall(L, 7, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        HAPLogError(&lhap_log, "%s: %s", __func__, lua_tostring(L, -1));
        return kHAPError_Unknown;
    }
    HAPAssert(lua_isinteger(L, -1));
    return lua_tointeger(L, -1);
}

static HAP_RESULT_USE_CHECK
HAPError lhap_char_Data_handleRead(
        HAPAccessoryServerRef* server,
        const HAPDataCharacteristicReadRequest* request,
        void* valueBytes,
        size_t maxValueBytes,
        size_t* numValueBytes,
        void* _Nullable context) {
    lua_State *L = app_get_lua_main_thread();
    HAPError err = lhap_char_base_handleRead(L, server, request->transportType,
        request->session, request->accessory, request->service,
        (const HAPBaseCharacteristic *)request->characteristic,
        &request->characteristic->callbacks.handleRead);

    if (err != kHAPError_None) {
        goto end;
    }

    size_t len;
    const char *data = lua_tolstring(L, -2, &len);
    if (len >= maxValueBytes) {
        HAPLogError(&lhap_log, "%s: value too long", __func__);
        err = kHAPError_OutOfResources;
        goto end;
    }
    *numValueBytes = len;
    if (len) {
        HAPRawBufferCopyBytes(valueBytes, data, len);
    }

end:
    lua_settop(L, 0);
    lc_collectgarbage(L);
    return err;
}

static HAP_RESULT_USE_CHECK
HAPError lhap_char_Bool_handleRead(
        HAPAccessoryServerRef* server,
        const HAPBoolCharacteristicReadRequest* request,
        bool* value,
        void* _Nullable context) {
    lua_State *L = app_get_lua_main_thread();
    HAPError err = lhap_char_base_handleRead(L, server, request->transportType,
        request->session, request->accessory, request->service,
        (const HAPBaseCharacteristic *)request->characteristic,
        &request->characteristic->callbacks.handleRead);

    if (err != kHAPError_None) {
        goto end;
    }

    *value = lua_toboolean(L, -2);
end:
    lua_settop(L, 0);
    lc_collectgarbage(L);
    return err;
}

static HAP_RESULT_USE_CHECK
HAPError lhap_char_number_handleRead(
        HAPAccessoryServerRef* server,
        HAPTransportType transportType,
        HAPSessionRef *session,
        const HAPAccessory *accessory,
        const HAPService *service,
        const HAPBaseCharacteristic *characteristic,
        void *value,
        const void *pfunc) {
    lua_State *L = app_get_lua_main_thread();
    HAPError err = lhap_char_base_handleRead(L, server, transportType, session,
        accessory, service, characteristic, pfunc);

    if (err != kHAPError_None) {
        goto end;
    }
    union lhap_char_value val;
    int valid = lhap_char_value_get(L, -2, characteristic->format, &val);
    if (!valid) {
        err = kHAPError_InvalidData;
        goto end;
    }
    switch (characteristic->format) {
    case kHAPCharacteristicFormat_UInt8:
        *((uint8_t *)value) = val.integer;
        break;
    case kHAPCharacteristicFormat_UInt16:
        *((uint16_t *)value) = val.integer;
        break;
    case kHAPCharacteristicFormat_UInt32:
        *((uint32_t *)value) = val.integer;
        break;
    case kHAPCharacteristicFormat_UInt64:
        *((uint64_t *)value) = val.integer;
        break;
    case kHAPCharacteristicFormat_Int:
        *((int32_t *)value) = val.integer;
        break;
    case kHAPCharacteristicFormat_Float:
        *((float *)value) = val.number;
        break;
    default:
        HAPAssertionFailure();
        break;
    }

end:
    lua_settop(L, 0);
    lc_collectgarbage(L);
    return err;
}

#define LHAP_CHAR_NUMBER_HANDLE_READ(format, vtype) \
static HAP_RESULT_USE_CHECK \
HAPError lhap_char_ ## format ## _handleRead( \
        HAPAccessoryServerRef* server, \
        const HAP ## format ##CharacteristicReadRequest* request, \
        vtype* value, \
        void* _Nullable context) { \
    return lhap_char_number_handleRead(server, \
        request->transportType, request->session, request->accessory, request->service, \
        (const HAPBaseCharacteristic *)request->characteristic, value, \
        &request->characteristic->callbacks.handleRead); \
}

LHAP_CHAR_NUMBER_HANDLE_READ(UInt8, uint8_t)
LHAP_CHAR_NUMBER_HANDLE_READ(UInt16, uint16_t)
LHAP_CHAR_NUMBER_HANDLE_READ(UInt32, uint32_t)
LHAP_CHAR_NUMBER_HANDLE_READ(UInt64, uint64_t)
LHAP_CHAR_NUMBER_HANDLE_READ(Int, int32_t)
LHAP_CHAR_NUMBER_HANDLE_READ(Float, float)

#undef LHAP_CHAR_NUMBER_HANDLE_READ

static HAP_RESULT_USE_CHECK
HAPError lhap_char_String_handleRead(
        HAPAccessoryServerRef* server,
        const HAPStringCharacteristicReadRequest* request,
        char* value,
        size_t maxValueBytes,
        void* _Nullable context) {
    lua_State *L = app_get_lua_main_thread();
    HAPError err = lhap_char_base_handleRead(L, server, request->transportType,
        request->session, request->accessory, request->service,
        (const HAPBaseCharacteristic *)request->characteristic,
        &request->characteristic->callbacks.handleRead);

    if (err != kHAPError_None) {
        goto end;
    }

    size_t len;
    const char *str = lua_tolstring(L, -2, &len);
    if (len >= maxValueBytes) {
        HAPLogError(&lhap_log, "%s: value too long", __func__);
        err = kHAPError_OutOfResources;
        goto end;
    }
    HAPRawBufferCopyBytes(value, str, len + 1);

end:
    lua_settop(L, 0);
    lc_collectgarbage(L);
    return err;
}

static HAP_RESULT_USE_CHECK
HAPError lhap_char_TLV8_handleRead(
        HAPAccessoryServerRef* server,
        const HAPTLV8CharacteristicReadRequest* request,
        HAPTLVWriterRef* responseWriter,
        void* _Nullable context) {
    lua_State *L = app_get_lua_main_thread();
    HAPError err = lhap_char_base_handleRead(L, server, request->transportType,
        request->session, request->accessory, request->service,
        (const HAPBaseCharacteristic *)request->characteristic,
        &request->characteristic->callbacks.handleRead);

    if (err != kHAPError_None) {
        goto end;
    }

    // TODO(Zebin Wu): Implement TLV8 in lua.
    HAPAssertionFailure();

end:
    lua_settop(L, 0);
    lc_collectgarbage(L);
    return err;
}

int lhap_char_handle_write_finsh(lua_State *L, int status, lua_KContext _ctx) {
    lhap_call_context *ctx = (lhap_call_context *)_ctx;
    HAPError err = kHAPError_None;
    if (status != LUA_OK && status != LUA_YIELD) {
        HAPLogError(&lhap_log, "%s: %s", __func__, lua_tostring(L, -1));
        err = kHAPError_Unknown;
    }
    if (ctx->in_progress == false) {
        lua_pushinteger(L, err);
        return 1;
    }
    err = HAPCharacteristicResponseWriteRequest(ctx->server, ctx->transportType,
        ctx->session, ctx->accessory, ctx->service, ctx->characteristic, err);
    if (err != kHAPError_None) {
        HAPLogError(&lhap_log, "%s: Failed to response write request, error code: %d.", __func__, err);
    }
    return 0;
}

static int lhap_char_handle_write(lua_State *L) {
    // stack: <call_ctx, traceback, func, request, value>
    lua_KContext call_ctx = (lua_KContext)lua_touserdata(L, 1);
    int status = lua_pcallk(L, 2, 0, 2, call_ctx, lhap_char_handle_write_finsh);
    return lhap_char_handle_write_finsh(L, status, call_ctx);
}

static int lhap_char_handle_write_pcall(lua_State *L) {
    HAPTransportType transportType = lua_tointeger(L, 2);
    HAPAccessoryServerRef *server = lua_touserdata(L, 3);
    HAPSessionRef *session = lua_touserdata(L, 4);
    const HAPAccessory *accessory = lua_touserdata(L, 5);
    const HAPService *service = lua_touserdata(L, 6);
    const HAPBaseCharacteristic *characteristic = lua_touserdata(L, 7);
    const void *pfunc = lua_touserdata(L, 8);
    bool remote = lua_toboolean(L, 9);
    lua_pop(L, 8);

    lua_State *co = lua_newthread(L);
    lua_pushcfunction(co, lhap_char_handle_write);
    lhap_call_context *call_ctx = lua_newuserdata(co, sizeof(*call_ctx));
    call_ctx->in_progress = false;
    call_ctx->transportType = transportType;
    call_ctx->server = server;
    call_ctx->session = session;
    call_ctx->accessory = accessory;
    call_ctx->service = service;
    call_ctx->characteristic = characteristic;

    lc_pushtraceback(co);
    HAPAssert(lua_rawgetp(co, LUA_REGISTRYINDEX, pfunc) == LUA_TFUNCTION);
    lhap_create_request_table(co, transportType, session, &remote,
        accessory, service, characteristic);

    lua_pushvalue(L, 1);
    lua_xmove(L, co, 1);
    lua_pop(L, 1);

    int status, nres;
    status = lc_resume(co, L, 5, &nres);
    switch (status) {
    case LUA_OK:
        HAPAssert(nres == 1);
        return 1;
    case LUA_YIELD:
        call_ctx->in_progress = true;
        lua_pushinteger(L, kHAPError_InProgress);
        return 1;
    default:
        return lua_error(L);
    }
}

static HAP_RESULT_USE_CHECK
HAPError lhap_char_base_handleWrite(
        lua_State *L,
        HAPAccessoryServerRef* server,
        HAPTransportType transportType,
        HAPSessionRef *session,
        bool remote,
        const HAPAccessory *accessory,
        const HAPService *service,
        const HAPBaseCharacteristic *characteristic,
        const void *pfunc) {
    lua_pushcfunction(L, lhap_char_handle_write_pcall);
    lua_insert(L, 1);
    lua_pushinteger(L, transportType);
    lua_pushlightuserdata(L, server);
    lua_pushlightuserdata(L, session);
    lua_pushlightuserdata(L, (void *)accessory);
    lua_pushlightuserdata(L, (void *)service);
    lua_pushlightuserdata(L, (void *)characteristic);
    lua_pushlightuserdata(L, (void *)pfunc);
    lua_pushboolean(L, remote);
    HAPError err = kHAPError_Unknown;
    int status = lua_pcall(L, 9, 1, 0);
    if (status != LUA_OK) {
        HAPLogError(&lhap_log, "%s: %s", __func__, lua_tostring(L, -1));
        goto end;
    }
    HAPAssert(lua_isinteger(L, -1));
    err = lua_tointeger(L, -1);

end:
    lua_settop(L, 0);
    lc_collectgarbage(L);
    return err;
}

static HAP_RESULT_USE_CHECK
HAPError lhap_char_Data_handleWrite(
        HAPAccessoryServerRef* server,
        const HAPDataCharacteristicWriteRequest* request,
        const void* valueBytes,
        size_t numValueBytes,
        void* _Nullable context) {
    lua_State *L = app_get_lua_main_thread();
    HAPAssert(lua_gettop(L) == 0);

    lua_pushlstring(L, valueBytes, numValueBytes);
    return lhap_char_base_handleWrite(L, server, request->transportType,
        request->session, request->remote, request->accessory,
        request->service, (const HAPBaseCharacteristic *)request->characteristic,
        &request->characteristic->callbacks.handleWrite);
}

static HAP_RESULT_USE_CHECK
HAPError lhap_char_Bool_handleWrite(
        HAPAccessoryServerRef* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context) {
    lua_State *L = app_get_lua_main_thread();
    HAPAssert(lua_gettop(L) == 0);

    lua_pushboolean(L, value);
    return lhap_char_base_handleWrite(L, server, request->transportType,
        request->session, request->remote, request->accessory,
        request->service, (const HAPBaseCharacteristic *)request->characteristic,
        &request->characteristic->callbacks.handleWrite);
}

HAP_RESULT_USE_CHECK
HAPError lhap_char_number_handleWrite(
        HAPAccessoryServerRef* server,
        HAPTransportType transportType,
        HAPSessionRef *session,
        bool remote,
        const HAPAccessory *accessory,
        const HAPService *service,
        const HAPBaseCharacteristic *characteristic,
        void *value,
        const void *pfunc) {
    lua_State *L = app_get_lua_main_thread();
    HAPAssert(lua_gettop(L) == 0);

    lua_Number num;
    switch (characteristic->format) {
    case kHAPCharacteristicFormat_UInt8:
        num = *((uint8_t *)value);
        break;
    case kHAPCharacteristicFormat_UInt16:
        num = *((uint16_t *)value);
        break;
    case kHAPCharacteristicFormat_UInt32:
        num = *((uint32_t *)value);
        break;
    case kHAPCharacteristicFormat_UInt64:
        num = *((uint64_t *)value);
        break;
    case kHAPCharacteristicFormat_Int:
        num = *((int32_t *)value);
        break;
    case kHAPCharacteristicFormat_Float:
        num = *((float *)value);
        break;
    default:
        HAPAssertionFailure();
        break;
    }
    lua_pushnumber(L, num);
    return lhap_char_base_handleWrite(L, server, transportType, session,
        remote, accessory, service, characteristic, pfunc);
}

#define LHAP_CHAR_NUMBER_HANDLE_WRITE(format, vtype) \
HAP_RESULT_USE_CHECK \
HAPError lhap_char_ ## format ## _handleWrite( \
        HAPAccessoryServerRef* server, \
        const HAP ## format ## CharacteristicWriteRequest* request, \
        vtype value, \
        void* _Nullable context) { \
    return lhap_char_number_handleWrite( \
        server, request->transportType, request->session, \
        request->remote, request->accessory, request->service, \
        (const HAPBaseCharacteristic *)request->characteristic, &value, \
        &request->characteristic->callbacks.handleWrite); \
}

LHAP_CHAR_NUMBER_HANDLE_WRITE(UInt8, uint8_t)
LHAP_CHAR_NUMBER_HANDLE_WRITE(UInt16, uint16_t)
LHAP_CHAR_NUMBER_HANDLE_WRITE(UInt32, uint32_t)
LHAP_CHAR_NUMBER_HANDLE_WRITE(UInt64, uint64_t)
LHAP_CHAR_NUMBER_HANDLE_WRITE(Int, int32_t)
LHAP_CHAR_NUMBER_HANDLE_WRITE(Float, float)

#undef LHAP_CHAR_NUMBER_HANDLE_WRITE

static HAP_RESULT_USE_CHECK
HAPError lhap_char_String_handleWrite(
        HAPAccessoryServerRef* server,
        const HAPStringCharacteristicWriteRequest* request,
        const char* value,
        void* _Nullable context) {
    lua_State *L = app_get_lua_main_thread();
    HAPAssert(lua_gettop(L) == 0);

    lua_pushstring(L, value);
    return lhap_char_base_handleWrite(L, server, request->transportType,
        request->session, request->remote, request->accessory, request->service,
        (const HAPBaseCharacteristic *)request->characteristic,
        &request->characteristic->callbacks.handleWrite);
}

static HAP_RESULT_USE_CHECK
HAPError lhap_char_TLV8_handleWrite(
        HAPAccessoryServerRef* server,
        const HAPTLV8CharacteristicWriteRequest* request,
        HAPTLVReaderRef* requestReader,
        void* _Nullable context) {
    lua_State *L = app_get_lua_main_thread();
    HAPAssert(lua_gettop(L) == 0);

    // TODO(Zebin Wu): Implement TLV8 in lua.
    HAPAssertionFailure();

    return lhap_char_base_handleWrite(L, server, request->transportType,
        request->session, request->remote, request->accessory, request->service,
        (const HAPBaseCharacteristic *)request->characteristic,
        &request->characteristic->callbacks.handleWrite);
}

// This definitation is only used to register characteristic callbacks.
#define LHAP_CASE_CHAR_REGISTER_CB(format, cb) \
LHAP_CASE_CHAR_FORMAT_CODE(format, arg, \
    lua_pushvalue(L, -1); \
    lua_rawsetp(L, LUA_REGISTRYINDEX, &p->callbacks.cb); \
    p->callbacks.cb = lhap_char_ ## format ## _ ## cb)

static bool
lhap_char_cbs_handle_read_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
#define LHAP_CASE_CHAR_REGISTER_READ_CB(format) \
    LHAP_CASE_CHAR_REGISTER_CB(format, handleRead)

    switch (((HAPBaseCharacteristic *)arg)->format) {
        LHAP_CASE_CHAR_REGISTER_READ_CB(Data)
        LHAP_CASE_CHAR_REGISTER_READ_CB(Bool)
        LHAP_CASE_CHAR_REGISTER_READ_CB(UInt8)
        LHAP_CASE_CHAR_REGISTER_READ_CB(UInt16)
        LHAP_CASE_CHAR_REGISTER_READ_CB(UInt32)
        LHAP_CASE_CHAR_REGISTER_READ_CB(UInt64)
        LHAP_CASE_CHAR_REGISTER_READ_CB(Int)
        LHAP_CASE_CHAR_REGISTER_READ_CB(Float)
        LHAP_CASE_CHAR_REGISTER_READ_CB(String)
        LHAP_CASE_CHAR_REGISTER_READ_CB(TLV8)
    }

#undef LHAP_CASE_CHAR_REGISTER_READ_CB

    return true;
}

static bool
lhap_char_cbs_handle_write_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
#define LHAP_CASE_CHAR_REGISTER_WRITE_CB(format) \
    LHAP_CASE_CHAR_REGISTER_CB(format, handleWrite)

    switch (((HAPBaseCharacteristic *)arg)->format) {
        LHAP_CASE_CHAR_REGISTER_WRITE_CB(Data)
        LHAP_CASE_CHAR_REGISTER_WRITE_CB(Bool)
        LHAP_CASE_CHAR_REGISTER_WRITE_CB(UInt8)
        LHAP_CASE_CHAR_REGISTER_WRITE_CB(UInt16)
        LHAP_CASE_CHAR_REGISTER_WRITE_CB(UInt32)
        LHAP_CASE_CHAR_REGISTER_WRITE_CB(UInt64)
        LHAP_CASE_CHAR_REGISTER_WRITE_CB(Int)
        LHAP_CASE_CHAR_REGISTER_WRITE_CB(Float)
        LHAP_CASE_CHAR_REGISTER_WRITE_CB(String)
        LHAP_CASE_CHAR_REGISTER_WRITE_CB(TLV8)
    }

#undef LHAP_CASE_CHAR_REGISTER_WRITE_CB

    return true;
}

static bool
lhap_char_cbs_handle_sub_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    return false;
}

static bool
lhap_char_cbs_handle_unsub_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    return false;
}

#undef LHAP_CASE_CHAR_REGISTER_CB

static const lc_table_kv lhap_char_cbs_kvs[] = {
    {"read", LC_TFUNCTION, lhap_char_cbs_handle_read_cb},
    {"write", LC_TFUNCTION, lhap_char_cbs_handle_write_cb},
    {"sub", LC_TFUNCTION, lhap_char_cbs_handle_sub_cb},
    {"unsub", LC_TFUNCTION, lhap_char_cbs_handle_unsub_cb},
    {NULL, LC_TNONE, NULL},
};

static bool
lhap_char_cbs_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    return lc_traverse_table(L, -1, lhap_char_cbs_kvs, arg);
}

static const lc_table_kv lhap_char_kvs[] = {
    {"format", LC_TSTRING, NULL},
    {"iid", LC_TNUMBER, lhap_char_iid_cb},
    {"type", LC_TSTRING, lhap_char_type_cb},
    {"mfgDesc", LC_TSTRING, lhap_char_mfg_desc_cb},
    {"props", LC_TTABLE, lhap_char_props_cb},
    {"units", LC_TSTRING, lhap_char_units_cb},
    {"constraints", LC_TTABLE, lhap_char_constraints_cb},
    {"cbs", LC_TTABLE, lhap_char_cbs_cb},
    {NULL, LC_TNONE, NULL},
};

static bool lhap_service_characteristics_arr_cb(lua_State *L, size_t i, void *arg) {
    HAPCharacteristic **characteristics = arg;

    if (!lua_istable(L, -1)) {
        return false;
    }

    lua_pushstring(L, "format");
    lua_gettable(L, -2);
    if (!lua_isstring(L, -1)) {
        char name[32];
        snprintf(name, sizeof(name), "chars[%zu].format", i + 1);
        LHAP_LOG_TYPE_ERROR(L, name, -1, LUA_TSTRING);
        return false;
    }
    int format = lhap_lookup_by_name(lua_tostring(L, -1),
        lhap_characteristic_format_strs,
        HAPArrayCount(lhap_characteristic_format_strs));
    lua_pop(L, 1);
    if (format == -1) {
        HAPLogError(&lhap_log, "%s: Invalid format.", __func__);
        return false;
    }

    HAPCharacteristic *c = pal_mem_calloc(lhap_characteristic_struct_size[format]);
    if (!c) {
        HAPLogError(&lhap_log, "%s: Failed to alloc memory.", __func__);
        return false;
    }
    ((HAPBaseCharacteristic *)c)->format = format;
    characteristics[i] = c;
    if (!lc_traverse_table(L, -1, lhap_char_kvs, c)) {
        HAPLogError(&lhap_log, "%s: Failed to parse characteristic.", __func__);
        return false;
    }
    return true;
}

static void lhap_reset_characteristic(lua_State *L, HAPCharacteristic *characteristic) {
    HAPCharacteristicFormat format =
        ((HAPBaseCharacteristic *)characteristic)->format;
    lhap_safe_free(((HAPBaseCharacteristic *)characteristic)->manufacturerDescription);

#define LHAP_RESET_CHAR_CBS(type, ptr) \
    LHAP_CASE_CHAR_FORMAT_CODE(type, ptr, \
        lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &p->callbacks.handleRead); \
        lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &p->callbacks.handleWrite); \
        lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &p->callbacks.handleSubscribe); \
        lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &p->callbacks.handleUnsubscribe); \
    )

    switch (format) {
    LHAP_RESET_CHAR_CBS(Data, characteristic)
    LHAP_RESET_CHAR_CBS(Bool, characteristic)
    LHAP_CASE_CHAR_FORMAT_CODE(UInt8, characteristic,
        if (p->constraints.validValues) {
            lhap_safe_free(p->constraints.validValues);
        }
        if (p->constraints.validValuesRanges) {
            lhap_safe_free(p->constraints.validValuesRanges);
        }
        lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &p->callbacks.handleRead);
        lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &p->callbacks.handleWrite);
        lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &p->callbacks.handleSubscribe);
        lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &p->callbacks.handleUnsubscribe);
    )
    LHAP_RESET_CHAR_CBS(UInt16, characteristic)
    LHAP_RESET_CHAR_CBS(UInt32, characteristic)
    LHAP_RESET_CHAR_CBS(UInt64, characteristic)
    LHAP_RESET_CHAR_CBS(Int, characteristic)
    LHAP_RESET_CHAR_CBS(Float, characteristic)
    LHAP_RESET_CHAR_CBS(String, characteristic)
    LHAP_RESET_CHAR_CBS(TLV8, characteristic)
    }

#undef LHAP_RESET_CHAR_CBS
}

static bool
lhap_service_chars_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPService *service = arg;
    HAPCharacteristic ***pcharacteristic = (HAPCharacteristic ***)&(service->characteristics);
    // Get the array length.
    lua_Unsigned len = lua_rawlen(L, -1);
    if (len == 0) {
        *pcharacteristic = NULL;
        return true;
    }

    HAPCharacteristic **characteristics = pal_mem_calloc((len + 1) * sizeof(HAPCharacteristic *));
    if (!characteristics) {
        HAPLogError(&lhap_log, "%s: Failed to alloc memory.", __func__);
        return false;
    }
    if (!lc_traverse_array(L, -1, lhap_service_characteristics_arr_cb, characteristics)) {
        HAPLogError(&lhap_log, "%s: Failed to parse characteristics.", __func__);
        for (HAPCharacteristic **c = characteristics; *c; c++) {
            lhap_reset_characteristic(L, *c);
            pal_mem_free(*c);
        }
        lhap_safe_free(characteristics);
        return false;
    }
    *pcharacteristic = characteristics;
    return true;
}

static const lc_table_kv lhap_service_kvs[] = {
    {"iid", LC_TNUMBER, lhap_service_iid_cb},
    {"type", LC_TSTRING, lhap_service_type_cb},
    {"props", LC_TTABLE, lhap_service_props_cb},
    {"linkedServices", LC_TTABLE, lhap_service_linked_services_cb},
    {"chars", LC_TTABLE, lhap_service_chars_cb},
    {NULL, LC_TNONE, NULL},
};

static void lhap_reset_service(lua_State *L, HAPService *service) {
    lhap_safe_free(service->name);
    lhap_safe_free(service->linkedServices);
    if (service->characteristics) {
        for (HAPCharacteristic **c = (HAPCharacteristic **)service->characteristics; *c; c++) {
            lhap_reset_characteristic(L, *c);
            pal_mem_free(*c);
        }
        lhap_safe_free(service->characteristics);
    }
}

static bool
lhap_accessory_services_arr_cb(lua_State *L, size_t i, void *arg) {
    HAPService **services = arg;

    if (lua_islightuserdata(L, -1)) {
        HAPService *s = lua_touserdata(L, -1);
        if (lhap_service_is_light_userdata(s)) {
            services[i] = s;
        } else {
            return false;
        }
        return true;
    }
    if (!lua_istable(L, -1)) {
        return false;
    }
    HAPService *s = pal_mem_calloc(sizeof(HAPService));
    if (!s) {
        HAPLogError(&lhap_log, "%s: Failed to alloc memory.", __func__);
        return false;
    }
    services[i] = s;
    if (!lc_traverse_table(L, -1, lhap_service_kvs, s)) {
        HAPLogError(&lhap_log, "%s: Failed to parse service.", __func__);
        return false;
    }
    return true;
}

static bool
lhap_accessory_services_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPAccessory *accessory = arg;
    HAPService ***pservices = (HAPService ***)&(accessory->services);

    // Get the array length.
    lua_Unsigned len = lua_rawlen(L, -1);
    if (len == 0) {
        *pservices = NULL;
        return true;
    }

    HAPService **services = pal_mem_calloc((len + 1) * sizeof(HAPService *));
    if (!services) {
        HAPLogError(&lhap_log, "%s: Failed to alloc memory.", __func__);
        return false;
    }

    if (!lc_traverse_array(L, -1, lhap_accessory_services_arr_cb, services)) {
        HAPLogError(&lhap_log, "%s: Failed to parse services.", __func__);
        for (HAPService **s = services; *s; s++) {
            if (!lhap_service_is_light_userdata(*s)) {
                lhap_reset_service(L, *s);
                pal_mem_free(*s);
            }
        }
        lhap_safe_free(services);
        return false;
    }

    *pservices = services;
    return true;
}

static int lhap_accessory_pcall_identify(lua_State *L) {
    const HAPAccessoryIdentifyRequest *request = lua_touserdata(L, 1);
    lua_pop(L, 1);

    lua_State *co = lua_newthread(L);
    const HAPAccessory *accessory = request->accessory;

    // push the identify function
    HAPAssert(lua_rawgetp(co, LUA_REGISTRYINDEX,
        &(accessory->callbacks.identify)) == LUA_TFUNCTION);

    // push the table request
    lhap_create_request_table(co, request->transportType,
        request->session, &request->remote, accessory, NULL, NULL);

    int status, nres;
    status = lc_resume(co, L, 1, &nres);
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        lua_error(L);
    }

    return 0;
}

static HAP_RESULT_USE_CHECK
HAPError lhap_accessory_on_identify(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPAccessoryIdentifyRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(context);

    lua_State *L = app_get_lua_main_thread();
    HAPAssert(lua_gettop(L) == 0);

    lua_pushcfunction(L, lhap_accessory_pcall_identify);
    lua_pushlightuserdata(L, (void *)request);
    int status = lua_pcall(L, 1, 0, 0);
    if (luai_unlikely(status != LUA_OK)) {
        HAPLogError(&lhap_log, "%s: %s", __func__, lua_tostring(L, -1));
        return kHAPError_Unknown;
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
    return kHAPError_None;
}

static bool
lhap_accessory_cbs_identify_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPAccessory *acc = arg;
    lua_pushvalue(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &(acc->callbacks.identify));
    acc->callbacks.identify = lhap_accessory_on_identify;
    return true;
}

static lc_table_kv lhap_accessory_cbs_kvs[] = {
    {"identify", LC_TFUNCTION, lhap_accessory_cbs_identify_cb},
    {NULL, LC_TNONE, NULL},
};

static bool
lhap_accessory_cbs_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    return lc_traverse_table(L, -1, lhap_accessory_cbs_kvs, arg);
}

static bool
lhap_accesory_context_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    lua_pushvalue(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, arg);
    return true;
}

static const lc_table_kv lhap_accessory_kvs[] = {
    {"aid", LC_TNUMBER, lhap_accessory_aid_cb},
    {"category", LC_TSTRING, lhap_accessory_category_cb},
    {"name", LC_TSTRING, lhap_accessory_name_cb},
    {"mfg", LC_TSTRING, lhap_accessory_mfg_cb},
    {"model", LC_TSTRING, lhap_accessory_model_cb},
    {"sn", LC_TSTRING, lhap_accessory_sn_cb},
    {"fwVer", LC_TSTRING, lhap_accessory_fw_ver_cb},
    {"hwVer", LC_TSTRING, lhap_accessory_hw_ver_cb},
    {"services", LC_TTABLE, lhap_accessory_services_cb},
    {"cbs", LC_TTABLE, lhap_accessory_cbs_cb},
    {"context", LC_TTABLE, lhap_accesory_context_cb},
    {NULL, LC_TNONE, NULL},
};

static void lhap_reset_accessory(lua_State *L, HAPAccessory *accessory) {
    lhap_safe_free(accessory->name);
    lhap_safe_free(accessory->manufacturer);
    lhap_safe_free(accessory->model);
    lhap_safe_free(accessory->serialNumber);
    lhap_safe_free(accessory->firmwareVersion);
    lhap_safe_free(accessory->hardwareVersion);
    if (accessory->services) {
        for (HAPService **s = (HAPService **)accessory->services; *s; s++) {
            if (!lhap_service_is_light_userdata(*s)) {
                lhap_reset_service(L, *s);
                pal_mem_free(*s);
            }
        }
        lhap_safe_free(accessory->services);
    }
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, accessory);
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &(accessory->callbacks.identify));
}

static void lhap_server_handle_update_state(HAPAccessoryServerRef *server, void *_Nullable context) {
    HAPPrecondition(context);
    HAPPrecondition(server);

    lhap_desc *desc = context;
    lua_State *L = app_get_lua_main_thread();

    HAPAssert(lua_gettop(L) == 0);
    lc_pushtraceback(L);
    HAPAssert(lua_rawgetp(L, LUA_REGISTRYINDEX, &desc->server_cbs.handleUpdatedState) == LUA_TFUNCTION);

    lua_pushstring(L, lhap_server_state_strs[HAPAccessoryServerGetState(server)]);
    if (lua_pcall(L, 1, 0, 1)) {
        HAPLogError(&lhap_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static void lhap_server_handle_session_accept(
        HAPAccessoryServerRef *server,
        HAPSessionRef *session,
        void *_Nullable context) {
    HAPPrecondition(context);
    HAPPrecondition(server);

    lhap_desc *desc = context;
    lua_State *L = app_get_lua_main_thread();

    HAPAssert(lua_gettop(L) == 0);
    lc_pushtraceback(L);
    HAPAssert(lua_rawgetp(L, LUA_REGISTRYINDEX, &desc->server_cbs.handleSessionAccept) == LUA_TFUNCTION);

    lua_pushlightuserdata(L, session);
    if (lua_pcall(L, 1, 0, 1)) {
        HAPLogError(&lhap_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static void lhap_server_handle_session_invalidate(
        HAPAccessoryServerRef *server,
        HAPSessionRef *session,
        void *_Nullable context) {
    HAPPrecondition(context);
    HAPPrecondition(server);

    lhap_desc *desc = context;
    lua_State *L = app_get_lua_main_thread();

    HAPAssert(lua_gettop(L) == 0);
    lc_pushtraceback(L);
    HAPAssert(lua_rawgetp(L, LUA_REGISTRYINDEX, &desc->server_cbs.handleSessionInvalidate) == LUA_TFUNCTION);

    lua_pushlightuserdata(L, session);
    if (lua_pcall(L, 1, 0, 1)) {
        HAPLogError(&lhap_log, "%s: %s", __func__, lua_tostring(L, -1));
    }
    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static bool
lhap_server_cb_update_state_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPAccessoryServerCallbacks *cbs = arg;

    lua_pushvalue(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &cbs->handleUpdatedState);
    cbs->handleUpdatedState = lhap_server_handle_update_state;
    return true;
}

static bool
lhap_server_cb_session_accept_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPAccessoryServerCallbacks *cbs = arg;

    lua_pushvalue(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &cbs->handleSessionAccept);
    cbs->handleSessionAccept = lhap_server_handle_session_accept;
    return true;
}

static bool
lhap_server_cb_session_invalid_cb(lua_State *L, const lc_table_kv *kv, void *arg) {
    HAPAccessoryServerCallbacks *cbs = arg;

    lua_pushvalue(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &cbs->handleSessionInvalidate);
    cbs->handleSessionInvalidate = lhap_server_handle_session_invalidate;
    return true;
}

static const lc_table_kv lhap_server_callbacks_kvs[] = {
    {"updatedState", LC_TFUNCTION, lhap_server_cb_update_state_cb},
    {"sessionAccept", LC_TFUNCTION, lhap_server_cb_session_accept_cb},
    {"sessionInvalidate", LC_TFUNCTION, lhap_server_cb_session_invalid_cb},
    {NULL, LC_TNONE, NULL},
};

static void lhap_reset_server_cb(lua_State *L, HAPAccessoryServerCallbacks *cbs) {
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &cbs->handleUpdatedState);
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &cbs->handleSessionAccept);
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &cbs->handleSessionInvalidate);
}

#if IP
static void
lhap_count_attr(const HAPAccessory *acc, size_t *attr, size_t *readable, size_t *writable, size_t *notify) {
    for (const HAPService * const *pserv = acc->services; *pserv; pserv++) {
        const HAPService *serv = *pserv;
        if (serv == &accessoryInformationService || serv == &pairingService ||
            serv == &hapProtocolInformationService) {
            continue;
        }
        (*attr)++;
        for (const HAPBaseCharacteristic * const *pchar =
            (const HAPBaseCharacteristic * const *)(*pserv)->characteristics; *pchar; pchar++) {
            const HAPCharacteristicProperties *props = &(*pchar)->properties;
            (*attr)++;
            if (props->readable) {
                (*readable)++;
            }
            if (props->writable) {
                (*writable)++;
            }
            if (props->supportsEventNotification) {
                (*notify)++;
            }
        }
    }
}
#endif

/**
 * init(primaryAccessory: table, serverCallbacks: table)
 *
 * If the category of the accessory is bridge, the parameters
 * bridgedAccessories is valid.
 */
static int lhap_init(lua_State *L) {
    lhap_desc *desc = &gv_lhap_desc;

    if (desc->inited) {
        lua_pushliteral(L, "HAP is already initialized.");
        goto err;
    }

    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TTABLE);

    desc->primary_acc = pal_mem_calloc(sizeof(HAPAccessory));
    if (!desc->primary_acc) {
        lua_pushliteral(L, "Failed to alloc memory.");
        goto err;
    }
    if (!lc_traverse_table(L, 1, lhap_accessory_kvs, desc->primary_acc)) {
        lua_pushliteral(L, "Failed to generate accessory structure from table accessory.");
        goto err1;
    }

    if (desc->primary_acc->aid != 1) {
        lua_pushliteral(L, "Primary accessory must have aid 1.");
        goto err1;
    }

    if (!lc_traverse_table(L, 2, lhap_server_callbacks_kvs, &desc->server_cbs)) {
        lua_pushliteral(L, "Failed to parse the server callbacks from table serverCallbacks.");
        goto err2;
    }

    HAPLog(&lhap_log,
        "Primary accessory \"%s\" has been configured.", desc->primary_acc->name);

    desc->inited = true;

    return 0;

err2:
    lhap_reset_server_cb(L, &desc->server_cbs);
err1:
    lhap_reset_accessory(L, desc->primary_acc);
    lhap_safe_free(desc->primary_acc);
err:
    lua_error(L);
    return 0;
}

/* deinit() */
int lhap_deinit(lua_State *L) {
    lhap_desc *desc = &gv_lhap_desc;

    if (!desc->inited) {
        luaL_error(L, "HAP is not initialized.");
    }

    if (desc->is_started) {
        luaL_error(L, "HAP is started.");
    }

    lhap_reset_server_cb(L, &desc->server_cbs);

    HAPRawBufferZero(&desc->server, sizeof(desc->server));
    HAPRawBufferZero(&desc->server_cbs, sizeof(desc->server_cbs));

    if (desc->bridged_accs) {
        for (HAPAccessory **pa = desc->bridged_accs; *pa != NULL; pa++) {
            lhap_reset_accessory(L, *pa);
            pal_mem_free(*pa);
        }
        lhap_safe_free(desc->bridged_accs);
    }
    if (desc->primary_acc) {
        lhap_reset_accessory(L, desc->primary_acc);
        lhap_safe_free(desc->primary_acc);
    }
    desc->bridged_accs_cnt = 0;
    desc->bridged_accs_max = 0;
    desc->inited = false;
    return 0;
}

/* addBridgedAccessory(accessory: table) */
static int lhap_add_bridged_accessory(lua_State *L) {
    lhap_desc *desc = &gv_lhap_desc;

    if (!desc->inited) {
        luaL_error(L, "HAP is not initialized.");
    }

    if (desc->is_started) {
        luaL_error(L, "HAP is already started");
    }

    luaL_checktype(L, 1, LUA_TTABLE);

    if (desc->bridged_accs_max - desc->bridged_accs_cnt <= 1) {
        desc->bridged_accs_max = desc->bridged_accs_cnt ? desc->bridged_accs_cnt * 2 : 2;
        HAPAccessory **accs = pal_mem_realloc(desc->bridged_accs, sizeof(HAPAccessory *) * desc->bridged_accs_max);
        if (!accs) {
            luaL_error(L, "Failed to alloc memory.");
        }
        accs[desc->bridged_accs_cnt] = NULL;
        desc->bridged_accs = accs;
    }

    HAPAccessory *acc = pal_mem_calloc(sizeof(HAPAccessory));
    if (!acc) {
        luaL_error(L, "Failed to alloc memory.");
    }
    if (!lc_traverse_table(L, 1, lhap_accessory_kvs, acc)) {
        lhap_reset_accessory(L, acc);
        pal_mem_free(acc);
        luaL_error(L, "Failed to generate accessory structure from table accessory.");
    }
    if (!HAPBridgedAccessoryIsValid(acc)) {
        lhap_reset_accessory(L, acc);
        pal_mem_free(acc);
        luaL_error(L, "Invalid bridged accessory.");
    }

    desc->bridged_accs[desc->bridged_accs_cnt++] = acc;
    desc->bridged_accs[desc->bridged_accs_cnt] = NULL;

    HAPLog(&lhap_log, "Bridged accessory \"%s\" has been configured.", acc->name);
    return 0;
}

/* start(confChanged: boolean) */
static int lhap_start(lua_State *L) {
    lhap_desc *desc = &gv_lhap_desc;

    if (!desc->inited) {
        luaL_error(L, "HAP is already initialized.");
    }

    if (desc->is_started) {
        luaL_error(L, "HAP is already started");
    }

    luaL_checktype(L, 1, LUA_TBOOLEAN);

    bool conf_changed = lua_toboolean(L, 1);

    if (desc->bridged_accs && desc->bridged_accs_max - desc->bridged_accs_cnt > 1) {
        size_t max = desc->bridged_accs_cnt + 1;
        HAPAccessory **accs = pal_mem_realloc(desc->bridged_accs, sizeof(HAPAccessory *) * max);
        if (!accs) {
            luaL_error(L, "Failed to resize bridged accessories.");
        }
        desc->bridged_accs = accs;
        desc->bridged_accs_max = max;
    }

    size_t attr_cnt = LHAP_ATTR_CNT_DFT;
    size_t readable_cnt = LHAP_CHAR_READ_CNT_DFT;
    size_t writable_cnt = LHAP_CHAR_WRITE_CNT_DFT;
    size_t notify_cnt = LHAP_CHAR_NOTIFY_CNT_DFT;

    lhap_count_attr(desc->primary_acc, &attr_cnt, &readable_cnt, &writable_cnt, &notify_cnt);

    if (desc->bridged_accs) {
        for (const HAPAccessory * const *pacc =
            (const HAPAccessory * const *)desc->bridged_accs; *pacc; pacc++) {
            lhap_count_attr(*pacc, &attr_cnt, &readable_cnt, &writable_cnt, &notify_cnt);
        }
    }

    if (readable_cnt == 0) {
        readable_cnt = 1;
    }
    if (writable_cnt == 0) {
        writable_cnt = 1;
    }
    if (notify_cnt == 0) {
        notify_cnt = 1;
    }

#if IP
    pal_hap_init_ip(&desc->server_options, readable_cnt, writable_cnt, notify_cnt);
#endif

#if BLE
    pal_hap_init_ble(&desc->server_options, attr_cnt);
#endif

    // Generate setup code, setup info and setup ID.
    pal_hap_acc_setup_gen(desc->platform->keyValueStore);

    // Display setup code.
    HAPSetupCode setupCode;
    HAPPlatformAccessorySetupLoadSetupCode(desc->platform->accessorySetup, &setupCode);
    HAPLog(&lhap_log, "Setup code: %s", setupCode.stringValue);

    // Initialize accessory server.
    HAPAccessoryServerCreate(
            &desc->server,
            &desc->server_options,
            desc->platform,
            &desc->server_cbs,
            desc);

    // Start accessory server.
    if (desc->bridged_accs) {
        HAPAccessoryServerStartBridge(&desc->server, desc->primary_acc,
        (const struct HAPAccessory *const *)desc->bridged_accs, conf_changed);
    } else {
        HAPAccessoryServerStart(&desc->server, desc->primary_acc);
    }

    desc->is_started = true;
    return 0;
}

/* stop() */
static int lhap_stop(lua_State *L) {
    lhap_desc *desc = &gv_lhap_desc;

    if (!desc->is_started) {
        luaL_error(L, "HAP is not started.");
    }

    // Stop accessory server.
    HAPAccessoryServerStop(&desc->server);

    // Release accessory server.
    HAPAccessoryServerRelease(&desc->server);

#if IP
    pal_hap_deinit_ip(&desc->server_options);
#endif

#if BLE
    pal_hap_deinit_ble(&desc->server_options);
#endif

    desc->is_started = false;

    return 0;
}

/**
 * raiseEvent(accessoryIID:integer, serviceIID:integer, characteristicIID:integer)
 */
static int lhap_raise_event(lua_State *L) {
    lua_Integer iid;
    HAPSessionRef *session = NULL;
    lhap_desc *desc = &gv_lhap_desc;

    if (!desc->is_started) {
        luaL_error(L, "HAP is not started.");
    }

    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    luaL_checktype(L, 3, LUA_TNUMBER);
    if (!lua_isnoneornil(L, 4)) {
        luaL_checktype(L, 4, LUA_TLIGHTUSERDATA);
        session = lua_touserdata(L, 4);
    }

    HAPAccessory *a = NULL;
    iid = lua_tointeger(L, 1);
    if (desc->primary_acc->aid == iid) {
        a = desc->primary_acc;
        goto service;
    }
    if (!desc->bridged_accs) {
        luaL_argerror(L, 1, "accessory not found");
    }
    for (HAPAccessory **pa = desc->bridged_accs; *pa != NULL; pa++) {
        if ((*pa)->aid == iid) {
            a = *pa;
            break;
        }
    }
    if (!a) {
        luaL_argerror(L, 1, "accessory not found");
    }

service:
    if (!a->services) {
        luaL_argerror(L, 2, "service not found");
    }
    HAPService *s = NULL;
    iid = lua_tointeger(L, 2);
    for (HAPService **ps = (HAPService **)a->services; *ps; ps++) {
        if ((*ps)->iid == iid) {
            s = *ps;
            break;
        }
    }
    if (!s) {
        luaL_argerror(L, 2, "service not found");
    }

    if (!s->characteristics) {
        luaL_argerror(L, 3, "characteristic not found");
    }
    HAPCharacteristic *c = NULL;
    iid = lua_tointeger(L, 3);
    for (HAPCharacteristic **pc = (HAPCharacteristic **)s->characteristics; *pc; pc++) {
        if ((*(HAPBaseCharacteristic **)pc)->iid == iid) {
            c = *pc;
            break;
        }
    }
    if (!c) {
        luaL_argerror(L, 3, "characteristic not found");
    }

    if (session) {
        HAPAccessoryServerRaiseEventOnSession(&desc->server, c, s, a, session);
    } else {
        HAPAccessoryServerRaiseEvent(&desc->server, c, s, a);
    }

    return 0;
}

static int lhap_get_new_iid(lua_State *L) {
    bool bridgedAcc = false;
    if (lua_gettop(L) == 1) {
        luaL_checktype(L, 1, LUA_TBOOLEAN);
        bridgedAcc = lua_toboolean(L, 1);
    }
    pal_nvs_handle *handle = pal_nvs_open(LHAP_NVS_NAMESPACE);
    if (luai_unlikely(!handle)) {
        luaL_error(L, "failed to open NVS handle");
    }
    const char *key = bridgedAcc ? "aid" : "iid";
    size_t len = pal_nvs_get_len(handle, key);
    uint64_t iid = bridgedAcc ? LHAP_BRIDGED_ACCESSORY_IID_DFT : (LHAP_ATTR_CNT_DFT + 1);
    if (len) {
        if (luai_unlikely(!pal_nvs_get(handle, key, &iid, sizeof(iid)))) {
            pal_nvs_close(handle);
            luaL_error(L, "failed to get iid from NVS");
        }
        iid += 1;
    }
    if (luai_unlikely(!pal_nvs_set(handle, key, &iid, sizeof(iid)))) {
        pal_nvs_close(handle);
        luaL_error(L, "failed to set iid to NVS");
    }
    pal_nvs_close(handle);
    lua_pushinteger(L, iid);
    return 1;
}

static const luaL_Reg haplib[] = {
    {"init", lhap_init},
    {"deinit", lhap_deinit},
    {"addBridgedAccessory", lhap_add_bridged_accessory},
    {"start", lhap_start},
    {"stop", lhap_stop},
    {"raiseEvent", lhap_raise_event},
    {"getNewInstanceID", lhap_get_new_iid},
    /* placeholders */
    {"AccessoryInformationService", NULL},
    {"HAPProtocolInformationService", NULL},
    {"PairingService", NULL},
    {NULL, NULL},
};

LUAMOD_API int luaopen_hap(lua_State *L) {
    luaL_newlib(L, haplib);

    /* set services */
    for (const lhap_lightuserdata *ud = lhap_accessory_services_userdatas;
        ud->ptr; ud++) {
        lua_pushlightuserdata(L, ud->ptr);
        lua_setfield(L, -2, ud->name);
    }

    return 1;
}

void lhap_set_platform(HAPPlatform *platform) {
    gv_lhap_desc.platform = platform;
}
