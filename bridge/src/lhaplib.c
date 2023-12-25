// Copyright (c) 2021-2022 Zebin Wu and homekit-bridge contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <lualib.h>
#include <lauxlib.h>
#include <pal/hap.h>
#include <pal/mem.h>
#include <pal/nvs.h>
#include <HAP.h>
#include <HAPCharacteristic.h>
#include <HAPAccessorySetup.h>

#include "app_int.h"
#include "lc.h"

#define LHAP_READ_REQUESTS_MAX 32

#define lhap_optfunction(L, n) luaL_opt(L, lhap_checkfunction, n, false)
#define lhap_optarray(L, n) luaL_opt(L, lhap_checkarray, n, 0)

#define LHAP_ACCESSORY_NAME "HAPAccessory*"
#define LHAP_SERVICE_NAME "HAPService*"
#define LHAP_CHARACTERISTIC_NAME "HAPCharacteristic*"
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

#define LHAP_BRIDGED_ACCESSORY_IID_DFT 2

#define LHAP_CASE_CHAR_FORMAT_CODE(format, ptr, code) \
    case kHAPCharacteristicFormat_ ## format: \
        { HAP ## format ## Characteristic *p = (HAP ## format ## Characteristic *)ptr; code; } break;

static const HAPLogObject lhap_log = {
    .subsystem = APP_BRIDGE_LOG_SUBSYSTEM,
    .category = "hap",
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
    "Video Doorbells",
    "AirPurifiers",
    "Heaters",
    "AirConditioners",
    "Humidifiers",
    "Dehumidifiers",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Sprinklers",
    "Faucets",
    "ShowerSystems",
    NULL
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
    NULL,
};

static const char *lhap_characteristic_units_strs[] = {
    "None",
    "Celsius",
    "ArcDegrees",
    "Percentage",
    "Lux",
    "Seconds",
    NULL,
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

typedef struct lhap_desc lhap_desc;

typedef struct lhap_read_request {
    lhap_desc *desc;
    HAPTransportType transportType;
    HAPSessionRef *session;
    const HAPAccessory *accessory;
    const HAPService *service;
    const HAPBaseCharacteristic *characteristic;
    const void *pfunc;
    struct lhap_read_request *next;
} lhap_read_request;

typedef struct lhap_desc {
    bool started;

    HAPAccessory *primary_acc;
    HAPAccessory **bridged_accs;
    size_t num_bridged_accs;

    lua_State *mL;
    lua_State *co;
    HAPPlatform platform;
    HAPAccessoryServerRef server;
    HAPAccessoryServerOptions server_options;
    HAPAccessoryServerCallbacks server_cbs;

    HAPPlatformTimerRef read_requests_timer;
    lhap_read_request *read_requests_head;
    lhap_read_request **read_requests_ptail;
    size_t num_read_requests;
    size_t max_read_requests;
} lhap_desc;

static lhap_desc gv_lhap_desc;

static bool lhap_checkfunction(lua_State *L, int arg) {
    luaL_checktype(L, arg, LUA_TFUNCTION);
    return true;
}

static size_t lhap_checkarray(lua_State *L, int arg) {
    luaL_checktype(L, arg, LUA_TTABLE);
    return lua_rawlen(L, arg);
}

static void
lhap_init_ip(HAPAccessoryServerOptions *options, size_t num_contexts, size_t num_notify) {
    HAPPrecondition(options);
    HAPPrecondition(num_contexts);
    HAPPrecondition(num_notify);

    static HAPIPAccessoryServerStorage serverStorage;

    size_t num_sessions = PAL_HAP_IP_SESSION_STORAGE_NUM_ELEMENTS;
    HAPIPSession *sessions = pal_mem_alloc(sizeof(HAPIPSession) * num_sessions);
    HAPAssert(sessions);
    char *inbounds = pal_mem_alloc(PAL_HAP_IP_SESSION_STORAGE_INBOUND_BUFSIZE * num_sessions);
    HAPAssert(inbounds);
    char *outbounds = pal_mem_alloc(PAL_HAP_IP_SESSION_STORAGE_OUTBOUND_BUFSIZE * num_sessions);
    HAPAssert(outbounds);
    char *scratches = pal_mem_alloc(PAL_HAP_IP_SESSION_STORAGE_SCRATCH_BUFSIZE * num_sessions);
    HAPAssert(scratches);
    HAPIPCharacteristicContextRef *contexts =
        pal_mem_alloc(sizeof(HAPIPCharacteristicContextRef) * num_contexts * num_sessions);
    HAPAssert(contexts);
    HAPIPEventNotificationRef *eventNotifications =
        pal_mem_alloc(sizeof(HAPIPEventNotificationRef) * num_notify * num_sessions);
    HAPAssert(eventNotifications);
    for (size_t i = 0; i < num_sessions; i++) {
        sessions[i].inboundBuffer.bytes = inbounds + PAL_HAP_IP_SESSION_STORAGE_INBOUND_BUFSIZE * i;
        sessions[i].inboundBuffer.numBytes = PAL_HAP_IP_SESSION_STORAGE_INBOUND_BUFSIZE;
        sessions[i].outboundBuffer.bytes = outbounds + PAL_HAP_IP_SESSION_STORAGE_OUTBOUND_BUFSIZE * i;
        sessions[i].outboundBuffer.numBytes = PAL_HAP_IP_SESSION_STORAGE_OUTBOUND_BUFSIZE;
        sessions[i].scratchBuffer.bytes = scratches + PAL_HAP_IP_SESSION_STORAGE_SCRATCH_BUFSIZE * i;
        sessions[i].scratchBuffer.numBytes = PAL_HAP_IP_SESSION_STORAGE_SCRATCH_BUFSIZE;
        sessions[i].contexts = contexts + num_contexts * i;
        sessions[i].numContexts = num_contexts;
        sessions[i].eventNotifications = eventNotifications + num_notify * i;
        sessions[i].numEventNotifications = num_notify;
    }
    serverStorage.sessions = sessions;
    serverStorage.numSessions = num_sessions;

    options->ip.transport = &kHAPAccessoryServerTransport_IP;
    options->ip.accessoryServerStorage = &serverStorage;
}

static void
lhap_deinit_ip(HAPAccessoryServerOptions *options) {
    HAPPrecondition(options);

    HAPIPAccessoryServerStorage *storage = options->ip.accessoryServerStorage;
    pal_mem_free(storage->sessions[0].inboundBuffer.bytes);
    pal_mem_free(storage->sessions[0].outboundBuffer.bytes);
    pal_mem_free(storage->sessions[0].scratchBuffer.bytes);
    pal_mem_free(storage->sessions[0].contexts);
    pal_mem_free(storage->sessions[0].eventNotifications);
    pal_mem_free(storage->sessions);
    storage->sessions = NULL;
    storage->numSessions = 0;
    HAPRawBufferZero(options, sizeof(*options));
}

static void lhap_rawsetp_reset(lua_State *L, int idx, const void *p) {
    lua_pushnil(L);
    lua_rawsetp(L, idx, p);
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

static bool lhap_char_props_readable_cb(lua_State *L, void *arg) {
    ((HAPCharacteristicProperties *)arg)->readable = lua_toboolean(L, -1);
    return true;
}

static bool lhap_char_props_writable_cb(lua_State *L, void *arg) {
    ((HAPCharacteristicProperties *)arg)->writable = lua_toboolean(L, -1);
    return true;
}

static bool lhap_char_props_support_evt_notify_cb(lua_State *L, void *arg) {
    ((HAPCharacteristicProperties *)arg)->supportsEventNotification = lua_toboolean(L, -1);
    return true;
}

static bool lhap_char_props_hidden_cb(lua_State *L, void *arg) {
    ((HAPCharacteristicProperties *)arg)->hidden = lua_toboolean(L, -1);
    return true;
}

static bool lhap_char_props_read_req_admin_pms_cb(lua_State *L, void *arg) {
    ((HAPCharacteristicProperties *)arg)->readRequiresAdminPermissions = lua_toboolean(L, -1);
    return true;
}

static bool lhap_char_props_write_req_admin_pms_cb(lua_State *L, void *arg) {
    ((HAPCharacteristicProperties *)arg)->writeRequiresAdminPermissions = lua_toboolean(L, -1);
    return true;
}

static bool lhap_char_props_req_timed_write_cb(lua_State *L, void *arg) {
    ((HAPCharacteristicProperties *)arg)->requiresTimedWrite = lua_toboolean(L, -1);
    return true;
}

static bool lhap_char_props_support_auth_data_cb(lua_State *L, void *arg) {
    ((HAPCharacteristicProperties *)arg)->supportsAuthorizationData = lua_toboolean(L, -1);
    return true;
}

static bool lhap_char_props_ip_control_point_cb(lua_State *L, void *arg) {
    ((HAPCharacteristicProperties *)arg)->ip.controlPoint = lua_toboolean(L, -1);
    return true;
}

static bool lhap_char_props_ip_support_write_resp_cb(lua_State *L, void *arg) {
    ((HAPCharacteristicProperties *)arg)->ip.supportsWriteResponse = lua_toboolean(L, -1);
    return true;
}

static const lc_table_kv lhap_char_props_ip_kvs[] = {
    {"controlPoint", LC_TBOOLEAN, lhap_char_props_ip_control_point_cb},
    {"supportsWriteResponse", LC_TBOOLEAN, lhap_char_props_ip_support_write_resp_cb},
    {NULL, LC_TNONE, NULL},
};

static bool lhap_char_props_ip_cb(lua_State *L, void *arg) {
    return lc_traverse_table(L, -1, lhap_char_props_ip_kvs, arg);
}

static bool lhap_char_props_ble_support_bc_notify_cb(lua_State *L, void *arg) {
    ((HAPCharacteristicProperties *)arg)->ble.supportsBroadcastNotification =
        lua_toboolean(L, -1);
    return true;
}

static bool lhap_char_props_ble_support_disconn_notify_cb(lua_State *L, void *arg) {
    ((HAPCharacteristicProperties *)arg)->ble.supportsDisconnectedNotification =
        lua_toboolean(L, -1);
    return true;
}

static bool lhap_char_props_ble_read_without_sec_cb(lua_State *L, void *arg) {
    ((HAPCharacteristicProperties *)arg)->ble.readableWithoutSecurity =
        lua_toboolean(L, -1);
    return true;
}

static bool lhap_char_props_ble_write_without_sec_cb(lua_State *L, void *arg) {
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

static bool lhap_char_props_ble_cb(lua_State *L, void *arg) {
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
    lhap_desc *desc;
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
            HAPFatalError();
        }
    }
    return valid;
}

static HAPError lhap_char_response_read_request(
        HAPAccessoryServerRef *server,
        HAPTransportType transportType,
        HAPSessionRef *session,
        const HAPAccessory *accessory,
        const HAPService *service,
        const HAPCharacteristic *characteristic,
        HAPError err,
        union lhap_char_value *val) {
    HAPPrecondition(err != kHAPError_None || val);

    switch (((HAPBaseCharacteristic *)characteristic)->format) {
    case kHAPCharacteristicFormat_Bool:
        err = HAPBoolCharacteristicResponseReadRequest(server, transportType,
            session, accessory, service, characteristic, err,
            err == kHAPError_None ? val->boolean : false);
        break;
    case kHAPCharacteristicFormat_UInt8:
        err = HAPUInt8CharacteristicResponseReadRequest(server, transportType,
            session, accessory, service, characteristic, err,
            err == kHAPError_None ? val->integer : 0);
        break;
    case kHAPCharacteristicFormat_UInt16:
        err = HAPUInt16CharacteristicResponseReadRequest(server, transportType,
            session, accessory, service, characteristic, err,
            err == kHAPError_None ? val->integer : 0);
        break;
    case kHAPCharacteristicFormat_UInt32:
        err = HAPUInt32CharacteristicResponseReadRequest(server, transportType,
            session, accessory, service, characteristic, err,
            err == kHAPError_None ? val->integer : 0);
        break;
    case kHAPCharacteristicFormat_UInt64:
        err = HAPUInt64CharacteristicResponseReadRequest(server, transportType,
            session, accessory, service, characteristic, err,
            err == kHAPError_None ? val->integer : 0);
        break;
    case kHAPCharacteristicFormat_Int:
        err = HAPIntCharacteristicResponseReadRequest(server, transportType,
            session, accessory, service, characteristic, err,
            err == kHAPError_None ? val->integer : 0);
        break;
    case kHAPCharacteristicFormat_Float:
        err = HAPFloatCharacteristicResponseReadRequest(server, transportType,
            session, accessory, service, characteristic, err,
            err == kHAPError_None ? val->number : 0);
        break;
    case kHAPCharacteristicFormat_Data: {
        err = HAPDataCharacteristicResponseReadRequest(server, transportType,
            session, accessory, service, characteristic, err,
            err == kHAPError_None ? val->str.data : NULL, err == kHAPError_None ? val->str.len : 0);
    } break;
    case kHAPCharacteristicFormat_String:
        err = HAPStringCharacteristicResponseReadRequest(server, transportType,
            session, accessory, service, characteristic, err,
            err == kHAPError_None ? val->str.data : NULL);
        break;
    case kHAPCharacteristicFormat_TLV8:
        // TODO(Zebin Wu): Implement TLV8 in lua.
        HAPFatalError();
    }
    return err;
}

static void lhap_schedule_read_requests_cb(HAPPlatformTimerRef timer, void* context);

int lhap_char_handle_read_finish(lua_State *L, int status, lua_KContext _ctx) {
    lhap_call_context *ctx = (lhap_call_context *)_ctx;
    lhap_desc *desc = ctx->desc;
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
    err = lhap_char_response_read_request(&desc->server, ctx->transportType, ctx->session,
        ctx->accessory, ctx->service, ctx->characteristic, err, &val);
    if (err != kHAPError_None) {
        HAPLogError(&lhap_log, "%s: Failed to response read request, error code: %d.", __func__, err);
    }
    desc->num_read_requests--;
    if (desc->num_read_requests == 0) {
        if (HAPPlatformTimerRegister(
                &ctx->desc->read_requests_timer,
                HAPPlatformClockGetCurrent(),
                lhap_schedule_read_requests_cb,
                desc)) {
            HAPLogError(&lhap_log, "%s: Failed to register schedule read requests timer.", __func__);
            HAPFatalError();
        }
    }
    lua_pushinteger(L, err);
    return 1;
}

static int lhap_char_handle_read(lua_State *L) {
    // stack: <call_ctx, traceback, func, request>
    lua_KContext call_ctx = (lua_KContext)lua_touserdata(L, 1);
    int status = lua_pcallk(L, 1, 1, 2, call_ctx, lhap_char_handle_read_finish);
    return lhap_char_handle_read_finish(L, status, call_ctx);
}

static int lhap_char_handle_read_pcall(lua_State *L) {
    lhap_call_context *_call_ctx  = lua_touserdata(L, 1);
    const void *pfunc = lua_touserdata(L, 2);
    lua_pop(L, 2);

    lua_State *co = lc_newthread(L);
    lua_pushcfunction(co, lhap_char_handle_read);
    lhap_call_context *call_ctx = lua_newuserdata(co, sizeof(*call_ctx));
    *call_ctx = *_call_ctx;

    lc_pushtraceback(co);

    // push the function
    HAPAssert(lua_rawgetp(co, LUA_REGISTRYINDEX, pfunc) == LUA_TFUNCTION);

    // push the table request
    lhap_create_request_table(co, call_ctx->transportType, call_ctx->session, NULL,
        call_ctx->accessory, call_ctx->service, call_ctx->characteristic);

    int status, nres;
    status = lc_resume(co, L, 4, &nres);
    switch (status) {
    case LUA_OK:
        return nres;
    case LUA_YIELD:
        call_ctx->in_progress = true;
        lua_pushinteger(L, kHAPError_InProgress);
        return 1;
    default:
        return lua_error(L);
    }
}

static HAP_RESULT_USE_CHECK
HAPError lhap_char_raw_handleRead(
        bool in_progress,
        lhap_desc *desc,
        HAPTransportType transportType,
        HAPSessionRef *session,
        const HAPAccessory *accessory,
        const HAPService *service,
        const HAPBaseCharacteristic *characteristic,
        const void *pfunc) {
    lua_State *L = desc->mL;
    HAPAssert(lua_gettop(L) == 0);

    desc->num_read_requests++;

    lhap_call_context call_ctx = {
        .in_progress = in_progress,
        .transportType = transportType,
        .desc = desc,
        .session = session,
        .accessory = accessory,
        .service = service,
        .characteristic = characteristic,
    };

    lua_pushcfunction(L, lhap_char_handle_read_pcall);
    lua_pushlightuserdata(L, (void *)&call_ctx);
    lua_pushlightuserdata(L, (void *)pfunc);
    int status = lua_pcall(L, 2, LUA_MULTRET, 0);
    if (status != LUA_OK) {
        HAPLogError(&lhap_log, "%s: %s", __func__, lua_tostring(L, -1));
        return kHAPError_Unknown;
    }
    HAPAssert(lua_isinteger(L, -1));
    HAPError err = lua_tointeger(L, -1);

    if (!in_progress && err != kHAPError_InProgress) {
        desc->num_read_requests--;
    }

    return err;
}

static void lhap_schedule_read_requests_cb(HAPPlatformTimerRef timer, void* context) {
    lhap_desc *desc = context;

    while (desc->read_requests_ptail != &desc->read_requests_head) {
        lhap_read_request *request = desc->read_requests_head;
        desc->read_requests_head = request->next;
        if (desc->read_requests_head == NULL) {
            desc->read_requests_ptail = &desc->read_requests_head;
        }
        HAPError err = lhap_char_raw_handleRead(true, desc, request->transportType, request->session,
            request->accessory, request->service, request->characteristic, request->pfunc);
        if (err != kHAPError_None && err != kHAPError_InProgress) {
            HAPLogError(&lhap_log, "%s: Failed to handle read request, error code: %d.", __func__, err);
            err = lhap_char_response_read_request(&desc->server, request->transportType, request->session,
                request->accessory, request->service, request->characteristic, err, NULL);
            if (err != kHAPError_None) {
                HAPLogError(&lhap_log, "%s: Failed to response read request, error code: %d.", __func__, err);
            }
        }
        pal_mem_free(request);
        lua_settop(desc->mL, 0);
        lc_collectgarbage(desc->mL);
    }
}

static HAP_RESULT_USE_CHECK
HAPError lhap_char_base_handleRead(
        lhap_desc *desc,
        HAPAccessoryServerRef *server,
        HAPTransportType transportType,
        HAPSessionRef *session,
        const HAPAccessory *accessory,
        const HAPService *service,
        const HAPBaseCharacteristic *characteristic,
        const void *pfunc) {
    lua_State *L = desc->mL;
    HAPAssert(lua_gettop(L) == 0);

    if (desc->num_read_requests == desc->max_read_requests) {
        lhap_read_request *request = pal_mem_alloc(sizeof(*request));
        if (!request) {
            return kHAPError_OutOfResources;
        }
        request->desc = desc;
        request->transportType = transportType;
        request->session = session;
        request->accessory = accessory;
        request->service = service,
        request->characteristic = characteristic;
        request->pfunc = pfunc;
        request->next = NULL;
        *(desc->read_requests_ptail) = request;
        desc->read_requests_ptail = &request->next;
        return kHAPError_InProgress;
    }

    return lhap_char_raw_handleRead(false, desc, transportType,
        session, accessory, service, characteristic, pfunc);
}

static HAP_RESULT_USE_CHECK
HAPError lhap_char_Data_handleRead(
        HAPAccessoryServerRef* server,
        const HAPDataCharacteristicReadRequest* request,
        void* valueBytes,
        size_t maxValueBytes,
        size_t* numValueBytes,
        void* _Nullable context) {
    lua_State *L = ((lhap_desc *)context)->mL;
    HAPError err = lhap_char_base_handleRead(context, server, request->transportType,
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
    lua_State *L = ((lhap_desc *)context)->mL;
    HAPError err = lhap_char_base_handleRead(context, server, request->transportType,
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
        lhap_desc *desc,
        HAPAccessoryServerRef* server,
        HAPTransportType transportType,
        HAPSessionRef *session,
        const HAPAccessory *accessory,
        const HAPService *service,
        const HAPBaseCharacteristic *characteristic,
        void *value,
        const void *pfunc) {
    lua_State *L = desc->mL;
    HAPError err = lhap_char_base_handleRead(desc, server, transportType, session,
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
        HAPFatalError();
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
    return lhap_char_number_handleRead(context, server, \
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
    lua_State *L = ((lhap_desc *)context)->mL;
    HAPError err = lhap_char_base_handleRead(context, server, request->transportType,
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
    lua_State *L = ((lhap_desc *)context)->mL;
    HAPError err = lhap_char_base_handleRead(context, server, request->transportType,
        request->session, request->accessory, request->service,
        (const HAPBaseCharacteristic *)request->characteristic,
        &request->characteristic->callbacks.handleRead);

    if (err != kHAPError_None) {
        goto end;
    }

    // TODO(Zebin Wu): Implement TLV8 in lua.
    HAPFatalError();

end:
    lua_settop(L, 0);
    lc_collectgarbage(L);
    return err;
}

int lhap_char_handle_write_finish(lua_State *L, int status, lua_KContext _ctx) {
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
    err = HAPCharacteristicResponseWriteRequest(&ctx->desc->server, ctx->transportType,
        ctx->session, ctx->accessory, ctx->service, ctx->characteristic, err);
    if (err != kHAPError_None) {
        HAPLogError(&lhap_log, "%s: Failed to response write request, error code: %d.", __func__, err);
    }
    return 0;
}

static int lhap_char_handle_write(lua_State *L) {
    // stack: <call_ctx, traceback, func, request, value>
    lua_KContext call_ctx = (lua_KContext)lua_touserdata(L, 1);
    int status = lua_pcallk(L, 2, 0, 2, call_ctx, lhap_char_handle_write_finish);
    return lhap_char_handle_write_finish(L, status, call_ctx);
}

static int lhap_char_handle_write_pcall(lua_State *L) {
    lhap_call_context *_call_ctx = lua_touserdata(L, 2);
    const void *pfunc = lua_touserdata(L, 3);
    bool remote = lua_toboolean(L, 4);
    lua_pop(L, 3);

    lua_State *co = lc_newthread(L);
    lua_pushcfunction(co, lhap_char_handle_write);
    lhap_call_context *call_ctx = lua_newuserdata(co, sizeof(*call_ctx));
    *call_ctx = *_call_ctx;

    lc_pushtraceback(co);
    HAPAssert(lua_rawgetp(co, LUA_REGISTRYINDEX, pfunc) == LUA_TFUNCTION);
    lhap_create_request_table(co, call_ctx->transportType, call_ctx->session, &remote,
        call_ctx->accessory, call_ctx->service, call_ctx->characteristic);

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
        lhap_desc *desc,
        HAPAccessoryServerRef* server,
        HAPTransportType transportType,
        HAPSessionRef *session,
        bool remote,
        const HAPAccessory *accessory,
        const HAPService *service,
        const HAPBaseCharacteristic *characteristic,
        const void *pfunc) {
    lua_State *L = desc->mL;

    lhap_call_context call_ctx = {
        .in_progress = false,
        .transportType = transportType,
        .desc = desc,
        .session = session,
        .accessory = accessory,
        .service = service,
        .characteristic = characteristic,
    };

    lua_pushcfunction(L, lhap_char_handle_write_pcall);
    lua_insert(L, 1);
    lua_pushlightuserdata(L, &call_ctx);
    lua_pushlightuserdata(L, (void *)pfunc);
    lua_pushboolean(L, remote);
    HAPError err = kHAPError_Unknown;
    int status = lua_pcall(L, 4, 1, 0);
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
    lua_State *L = ((lhap_desc *)context)->mL;
    HAPAssert(lua_gettop(L) == 0);

    lua_pushlstring(L, valueBytes, numValueBytes);
    return lhap_char_base_handleWrite(context, server, request->transportType,
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
    lua_State *L = ((lhap_desc *)context)->mL;
    HAPAssert(lua_gettop(L) == 0);

    lua_pushboolean(L, value);
    return lhap_char_base_handleWrite(context, server, request->transportType,
        request->session, request->remote, request->accessory,
        request->service, (const HAPBaseCharacteristic *)request->characteristic,
        &request->characteristic->callbacks.handleWrite);
}

HAP_RESULT_USE_CHECK
HAPError lhap_char_number_handleWrite(
        lhap_desc *desc,
        HAPAccessoryServerRef* server,
        HAPTransportType transportType,
        HAPSessionRef *session,
        bool remote,
        const HAPAccessory *accessory,
        const HAPService *service,
        const HAPBaseCharacteristic *characteristic,
        void *value,
        const void *pfunc) {
    lua_State *L = desc->mL;
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
        HAPFatalError();
    }
    lua_pushnumber(L, num);
    return lhap_char_base_handleWrite(desc, server, transportType, session,
        remote, accessory, service, characteristic, pfunc);
}

#define LHAP_CHAR_NUMBER_HANDLE_WRITE(format, vtype) \
HAP_RESULT_USE_CHECK \
HAPError lhap_char_ ## format ## _handleWrite( \
        HAPAccessoryServerRef* server, \
        const HAP ## format ## CharacteristicWriteRequest* request, \
        vtype value, \
        void* _Nullable context) { \
    return lhap_char_number_handleWrite(context, \
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
    lua_State *L = ((lhap_desc *)context)->mL;
    HAPAssert(lua_gettop(L) == 0);

    lua_pushstring(L, value);
    return lhap_char_base_handleWrite(context, server, request->transportType,
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
    lua_State *L = ((lhap_desc *)context)->mL;
    HAPAssert(lua_gettop(L) == 0);

    // TODO(Zebin Wu): Implement TLV8 in lua.
    HAPFatalError();

    return lhap_char_base_handleWrite(context, server, request->transportType,
        request->session, request->remote, request->accessory, request->service,
        (const HAPBaseCharacteristic *)request->characteristic,
        &request->characteristic->callbacks.handleWrite);
}

// This definitation is only used to register characteristic callbacks.
#define LHAP_CASE_CHAR_REGISTER_CB(L, arg, ptr, format, cb) \
LHAP_CASE_CHAR_FORMAT_CODE(format, ptr, \
    lua_pushvalue(L, arg); \
    lua_rawsetp(L, LUA_REGISTRYINDEX, &p->callbacks.cb); \
    p->callbacks.cb = lhap_char_ ## format ## _ ## cb)

static int lhap_accessory_pcall_identify(lua_State *L) {
    const HAPAccessoryIdentifyRequest *request = lua_touserdata(L, 1);
    lua_pop(L, 1);

    lua_State *co = lc_newthread(L);
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

    lua_State *L = ((lhap_desc *)context)->mL;
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

static int lhap_server_handle_session_pcall(lua_State *L) {
    HAPSessionRef *session = lua_touserdata(L, 1);
    const void *pfunc = lua_touserdata(L, 2);
    lua_pop(L, 2);

    lua_State *co = lc_newthread(L);
    lua_rawgetp(co, LUA_REGISTRYINDEX, pfunc);
    lua_pushlightuserdata(co, session);
    int status, nres;
    status = lc_resume(co, L, 1, &nres);
    if (luai_unlikely(status != LUA_OK && status != LUA_YIELD)) {
        lua_error(L);
    }
    return 0;
}

static void lhap_server_handle_session_accept(
        HAPAccessoryServerRef *server,
        HAPSessionRef *session,
        void *_Nullable context) {
    HAPPrecondition(context);
    HAPPrecondition(server);

    lhap_desc *desc = context;
    HAPAssert(&desc->server == server);
    lua_State *L = desc->mL;

    HAPAssert(lua_gettop(L) == 0);

    lua_pushcfunction(L, lhap_server_handle_session_pcall);
    lua_pushlightuserdata(L, session);
    lua_pushlightuserdata(L, &desc->server_cbs.handleSessionAccept);
    int status = lua_pcall(L, 2, 0, 0);
    if (status != LUA_OK) {
        HAPLogError(&lhap_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static void lhap_server_handle_session_invalid(
        HAPAccessoryServerRef *server,
        HAPSessionRef *session,
        void *_Nullable context) {
    HAPPrecondition(context);
    HAPPrecondition(server);

    lhap_desc *desc = context;
    HAPAssert(&desc->server == server);
    lua_State *L = desc->mL;

    HAPAssert(lua_gettop(L) == 0);

    lua_pushcfunction(L, lhap_server_handle_session_pcall);
    lua_pushlightuserdata(L, session);
    lua_pushlightuserdata(L, &desc->server_cbs.handleSessionInvalidate);
    int status = lua_pcall(L, 2, 0, 0);
    if (status != LUA_OK) {
        HAPLogError(&lhap_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static void lhap_reset_server_cb(lua_State *L, HAPAccessoryServerCallbacks *cbs) {
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &cbs->handleSessionAccept);
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &cbs->handleSessionInvalidate);
}

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

static void lhap_server_handle_update_state(HAPAccessoryServerRef *server, void *_Nullable context) {
    HAPPrecondition(context);
    HAPPrecondition(server);

    lhap_desc *desc = context;
    HAPAssert(&desc->server == server);
    lua_State *L = desc->mL;
    lua_State *co = desc->co;
    if (co == NULL) {
        return;
    }
    switch (HAPAccessoryServerGetState(&desc->server)) {
    case kHAPAccessoryServerState_Idle:
        HAPLog(&kHAPLog_Default, "Accessory Server State did update: Idle.");
        HAPAssert(desc->started == true);
        break;
    case kHAPAccessoryServerState_Running:
        HAPLog(&kHAPLog_Default, "Accessory Server State did update: Running.");
        HAPAssert(desc->started == false);
        break;
    case kHAPAccessoryServerState_Stopping:
        HAPLog(&kHAPLog_Default, "Accessory Server State did update: Stopping.");
        return;
    }
    HAPAssert(lua_gettop(L) == 0);

    int status, nres;
    status = lc_resume(co, L, 0, &nres);
    if (status != LUA_OK && status != LUA_YIELD) {
        HAPLogError(&lhap_log, "%s: %s", __func__, lua_tostring(L, -1));
    }

    lua_settop(L, 0);
    lc_collectgarbage(L);
}

static int lhap_new_accessory(lua_State *L) {
    uint64_t aid = luaL_checkinteger(L, 1);
    HAPAccessoryCategory category = luaL_checkoption(L, 2, NULL, lhap_accessory_category_strs);
    const char *name = luaL_checkstring(L, 3);
    const char *mfg = luaL_checkstring(L, 4);
    const char *model = luaL_checkstring(L, 5);
    const char *sn = luaL_checkstring(L, 6);
    const char *fwVer = luaL_checkstring(L, 7);
    const char *hwVer = luaL_optstring(L, 8, NULL);
    size_t nservices = lhap_checkarray(L, 9);
    luaL_argcheck(L, nservices, 9, "empty services");
    bool has_identify = lhap_optfunction(L, 10);

    HAPAccessory *accessory = lua_newuserdatauv(L, sizeof(HAPAccessory), 7);
    luaL_setmetatable(L, LHAP_ACCESSORY_NAME);
    for (size_t i = 3, j = 1; i <= 8; i++, j++) {
        lua_pushvalue(L, i);
        lua_setiuservalue(L, -2, j);
    }

    accessory->aid = aid;
    accessory->category = category;
    accessory->name = name;
    accessory->manufacturer = mfg;
    accessory->model = model;
    accessory->serialNumber = sn;
    accessory->firmwareVersion = fwVer;
    accessory->hardwareVersion = hwVer;

    HAPService **services = lua_newuserdata(L, sizeof(HAPService *) * (nservices + 1));
    lua_pushvalue(L, 9);
    lua_setuservalue(L, -2);
    lua_setiuservalue(L, -2, 7);
    for (size_t i = 1; i <= nservices; i++) {
        int type = lua_geti(L, 9, i);
        if (luai_unlikely(type != LUA_TUSERDATA && type != LUA_TLIGHTUSERDATA)) {
            luaL_error(L, "services[%d]: invalid type", i);
        }
        services[i - 1] = lua_touserdata(L, -1);
        lua_pop(L, 1);
    }
    services[nservices] = NULL;
    accessory->services = (const HAPService * const *)services;

    accessory->callbacks.identify = has_identify ? lhap_accessory_on_identify : NULL;
    if (has_identify) {
        lua_pushvalue(L, 10);
        lua_rawsetp(L, LUA_REGISTRYINDEX, &(accessory->callbacks.identify));
    }
    return 1;
}

static int lhap_accessory_gc(lua_State *L) {
    HAPAccessory *accessory = luaL_checkudata(L, 1, LHAP_ACCESSORY_NAME);

    if (accessory->callbacks.identify) {
        lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &(accessory->callbacks.identify));
    }
    return 0;
}

static const lhap_service_type *lhap_get_service_type(const char *s) {
    for (int i = 0; i < HAPArrayCount(lhap_service_type_tab);
        i++) {
        if (HAPStringAreEqual(s, lhap_service_type_tab[i].name)) {
            return lhap_service_type_tab + i;
        }
    }
    return NULL;
}

static int lhap_new_service(lua_State *L) {
    uint64_t iid = luaL_checkinteger(L, 1);
    const lhap_service_type *type = lhap_get_service_type(luaL_checkstring(L, 2));
    luaL_argcheck(L, type, 2, "unknown type");
    luaL_checktype(L, 3, LUA_TBOOLEAN);
    luaL_checktype(L, 4, LUA_TBOOLEAN);
    size_t nchars = lhap_checkarray(L, 5);
    luaL_argcheck(L, nchars, 5, "empty characteristics");

    HAPService *service = lua_newuserdatauv(L, sizeof(HAPService), 2);
    luaL_setmetatable(L, LHAP_SERVICE_NAME);
    HAPRawBufferZero(service, sizeof(HAPService));
    service->iid = iid;
    service->serviceType = type->type;
    service->debugDescription = type->debugDescription;
    service->name = NULL;
    service->properties.primaryService = lua_toboolean(L, 3);
    service->properties.hidden = lua_toboolean(L, 4);
    service->properties.ble.supportsConfiguration = false;

    HAPCharacteristic **characteristics = lua_newuserdata(L, sizeof(HAPCharacteristic *) * (nchars + 1));
    lua_pushvalue(L, 5);
    lua_setuservalue(L, -2);
    lua_setiuservalue(L, -2, 2);
    for (size_t i = 1; i <= nchars; i++) {
        lua_geti(L, 5, i);
        characteristics[i - 1] = luaL_testudata(L, -1, LHAP_CHARACTERISTIC_NAME);
        if (luai_unlikely(!characteristics[i - 1])) {
            luaL_error(L, "characteristics[%d]: invalid type", i);
        }
        lua_pop(L, 1);
    }
    characteristics[nchars] = NULL;
    service->characteristics = (const HAPCharacteristic * const *)characteristics;

    return 1;
}

static int lhap_service_link_services(lua_State *L) {
    HAPService *service = luaL_checkudata(L, 1, LHAP_SERVICE_NAME);
    if (luai_unlikely(service->linkedServices)) {
        luaL_error(L, "already link services");
    }
    int nargs = lua_gettop(L);
    if (nargs == 1) {
        luaL_error(L, "missing iids");
    }
    for (int i = 2; i <= nargs; i++) {
        luaL_checkinteger(L, i);
    }
    size_t niids = nargs - 1;

    uint64_t *iids = lua_newuserdatauv(L, sizeof(uint64_t) * (niids + 1), 0);
    lua_setiuservalue(L, 1, 1);
    for (int i = niids - 1; i >= 0; i--) {
        iids[i] =  lua_tointeger(L, -1);
        lua_pop(L, 1);
    }
    iids[niids] = 0;
    service->linkedServices = iids;

    return 1;
}

static const lhap_characteristic_type *lhap_get_char_type(const char *s) {
    for (int i = 0; i < HAPArrayCount(lhap_characteristic_type_tab);
        i++) {
        if (HAPStringAreEqual(s, lhap_characteristic_type_tab[i].name)) {
            return lhap_characteristic_type_tab + i;
        }
    }
    return NULL;
}

static int lhap_new_char(lua_State *L) {
    uint64_t iid = luaL_checkinteger(L, 1);
    HAPCharacteristicFormat format = luaL_checkoption(L, 2, NULL, lhap_characteristic_format_strs);
    const lhap_characteristic_type *type = lhap_get_char_type(luaL_checkstring(L, 3));
    luaL_argcheck(L, type, 3, "unknown type");
    luaL_checktype(L, 4, LUA_TTABLE);
    bool has_read = lhap_optfunction(L, 5);
    bool has_write = lhap_optfunction(L, 6);

    HAPBaseCharacteristic *characteristic = lua_newuserdatauv(L,
        lhap_characteristic_struct_size[format], format == kHAPCharacteristicFormat_UInt8 ? 3 : 1);
    luaL_setmetatable(L, LHAP_CHARACTERISTIC_NAME);
    HAPRawBufferZero(characteristic, lhap_characteristic_struct_size[format]);
    characteristic->iid = iid;
    characteristic->format = format;
    characteristic->characteristicType = type->type;
    characteristic->debugDescription = type->debugDescription;
    lc_traverse_table(L, 4, lhap_char_props_kvs, &characteristic->properties);

    if (has_read) {
#define LHAP_CASE_CHAR_REGISTER_READ_CB(format) \
    LHAP_CASE_CHAR_REGISTER_CB(L, 5, characteristic, format, handleRead)

    switch (format) {
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
    }

    if (has_write) {
#define LHAP_CASE_CHAR_REGISTER_WRITE_CB(format) \
    LHAP_CASE_CHAR_REGISTER_CB(L, 6, characteristic, format, handleWrite)

    switch (format) {
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

#undef LHAP_CASE_CHAR_REGISTER_READ_CB
    }

    // TODO(Zebin Wu): Register sub/unsub callbacks.
    return 1;
}

static int lhap_char_gc(lua_State *L) {
    HAPBaseCharacteristic *characteristic = luaL_checkudata(L, 1, LHAP_CHARACTERISTIC_NAME);

#define LHAP_RESET_CHAR_CBS(type, ptr) \
    LHAP_CASE_CHAR_FORMAT_CODE(type, ptr, \
        lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &p->callbacks.handleRead); \
        lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &p->callbacks.handleWrite); \
        lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &p->callbacks.handleSubscribe); \
        lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &p->callbacks.handleUnsubscribe); \
    )

    switch (characteristic->format) {
    LHAP_RESET_CHAR_CBS(Data, characteristic)
    LHAP_RESET_CHAR_CBS(Bool, characteristic)
    LHAP_RESET_CHAR_CBS(UInt8, characteristic)
    LHAP_RESET_CHAR_CBS(UInt16, characteristic)
    LHAP_RESET_CHAR_CBS(UInt32, characteristic)
    LHAP_RESET_CHAR_CBS(UInt64, characteristic)
    LHAP_RESET_CHAR_CBS(Int, characteristic)
    LHAP_RESET_CHAR_CBS(Float, characteristic)
    LHAP_RESET_CHAR_CBS(String, characteristic)
    LHAP_RESET_CHAR_CBS(TLV8, characteristic)
    }

#undef LHAP_RESET_CHAR_CBS

    return 0;
}

static int lhap_char_set_mfg_desc(lua_State *L) {
    HAPBaseCharacteristic *characteristic = luaL_checkudata(L, 1, LHAP_CHARACTERISTIC_NAME);
    const char *mfgDesc = luaL_checkstring(L, 2);
    lua_setiuservalue(L, 1, 1);
    characteristic->manufacturerDescription = mfgDesc;
    return 1;
}

static int lhap_char_set_units(lua_State *L) {
    HAPBaseCharacteristic *characteristic = luaL_checkudata(L, 1, LHAP_CHARACTERISTIC_NAME);
    HAPCharacteristicFormat format = characteristic->format;
    if (format < kHAPCharacteristicFormat_UInt8 ||
        format > kHAPCharacteristicFormat_Float) {
        luaL_error(L, "%s characteristic has no units.", lhap_characteristic_format_strs[format]);
    }
    HAPCharacteristicUnits units = luaL_checkoption(L, 2, "None", lhap_characteristic_units_strs);

    switch (format) {
    LHAP_CASE_CHAR_FORMAT_CODE(UInt8, characteristic, p->units = units)
    LHAP_CASE_CHAR_FORMAT_CODE(UInt16, characteristic, p->units = units)
    LHAP_CASE_CHAR_FORMAT_CODE(UInt32, characteristic, p->units = units)
    LHAP_CASE_CHAR_FORMAT_CODE(UInt64, characteristic, p->units = units)
    LHAP_CASE_CHAR_FORMAT_CODE(Int, characteristic, p->units = units)
    LHAP_CASE_CHAR_FORMAT_CODE(Float, characteristic, p->units = units)
    default:
        HAPFatalError();
    }
    lua_pop(L, 1);
    return 1;
}

static int lhap_char_set_contraints(lua_State *L) {
    HAPBaseCharacteristic *characteristic = luaL_checkudata(L, 1, LHAP_CHARACTERISTIC_NAME);
    HAPCharacteristicFormat format = characteristic->format;
    switch (format) {
    LHAP_CASE_CHAR_FORMAT_CODE(String, characteristic, p->constraints.maxLength = luaL_checkinteger(L, 2))
    LHAP_CASE_CHAR_FORMAT_CODE(Data, characteristic, p->constraints.maxLength = luaL_checkinteger(L, 2))
    LHAP_CASE_CHAR_FORMAT_CODE(UInt8, characteristic,
        p->constraints.minimumValue = luaL_checkinteger(L, 2);
        p->constraints.maximumValue = luaL_checkinteger(L, 3);
        p->constraints.stepValue = luaL_checkinteger(L, 4);
    )
    LHAP_CASE_CHAR_FORMAT_CODE(UInt16, characteristic,
        p->constraints.minimumValue = luaL_checkinteger(L, 2);
        p->constraints.maximumValue = luaL_checkinteger(L, 3);
        p->constraints.stepValue = luaL_checkinteger(L, 4);
    )
    LHAP_CASE_CHAR_FORMAT_CODE(UInt32, characteristic,
        p->constraints.minimumValue = luaL_checkinteger(L, 2);
        p->constraints.maximumValue = luaL_checkinteger(L, 3);
        p->constraints.stepValue = luaL_checkinteger(L, 4);
    )
    LHAP_CASE_CHAR_FORMAT_CODE(UInt64, characteristic,
        p->constraints.minimumValue = luaL_checkinteger(L, 2);
        p->constraints.maximumValue = luaL_checkinteger(L, 3);
        p->constraints.stepValue = luaL_checkinteger(L, 4);
    )
    LHAP_CASE_CHAR_FORMAT_CODE(Int, characteristic,
        p->constraints.minimumValue = luaL_checkinteger(L, 2);
        p->constraints.maximumValue = luaL_checkinteger(L, 3);
        p->constraints.stepValue = luaL_checkinteger(L, 4);
    )
    LHAP_CASE_CHAR_FORMAT_CODE(Float, characteristic,
        p->constraints.minimumValue = luaL_checknumber(L, 2);
        p->constraints.maximumValue = luaL_checknumber(L, 3);
        p->constraints.stepValue = luaL_checknumber(L, 4);
    )
    default:
        HAPFatalError();
    }
    lua_pushvalue(L, 1);
    return 1;
}

static int lhap_char_set_valid_vals(lua_State *L) {
    HAPBaseCharacteristic *_characteristic = luaL_checkudata(L, 1, LHAP_CHARACTERISTIC_NAME);
    if (_characteristic->format != kHAPCharacteristicFormat_UInt8) {
        luaL_error(L, "attempt to set valid values for not 'UInt8' format characteristic");
    }
    HAPUInt8Characteristic *characteristic = (HAPUInt8Characteristic *)_characteristic;
    if (characteristic->constraints.validValues) {
        luaL_error(L, "already set valid values");
    }

    int nargs = lua_gettop(L);
    if (nargs == 1) {
        luaL_error(L, "missing values");
    }
    for (int i = 2; i <= nargs; i++) {
        luaL_checkinteger(L, i);
    }
    size_t nvals = nargs - 1;

    size_t list_len = sizeof(uint8_t *) * (nvals + 1);
    uint8_t **list = lua_newuserdatauv(L, list_len + sizeof(uint8_t) * nvals, 0);
    lua_setiuservalue(L, 1, 2);
    characteristic->constraints.validValues = (const uint8_t * const *)list;

    uint8_t *vals = (uint8_t *)((uintptr_t)list + list_len);
    for (size_t i = 0; i < nvals; i++) {
        list[i] = vals + i;
    }
    list[nvals] = NULL;

    for (int i = nvals - 1; i >= 0; i--) {
        vals[i] = lua_tointeger(L, -1);
        lua_pop(L, 1);
    }

    return 1;
}

static int lhap_set_valid_vals_ranges(lua_State *L) {
    HAPBaseCharacteristic *_characteristic = luaL_checkudata(L, 1, LHAP_CHARACTERISTIC_NAME);
    if (_characteristic->format != kHAPCharacteristicFormat_UInt8) {
        luaL_error(L, "attempt to set valid values for not 'UInt8' format characteristic");
    }
    HAPUInt8Characteristic *characteristic = (HAPUInt8Characteristic *)_characteristic;
    if (characteristic->constraints.validValuesRanges) {
        luaL_error(L, "already set valid values");
    }

    int nargs = lua_gettop(L);
    if (nargs == 1) {
        luaL_error(L, "missing values ranges");
    }
    for (int i = 2; i <= nargs; i++) {
        size_t n = lhap_checkarray(L, i);
        luaL_argcheck(L, n == 2, i, "length must be 2");
        for (int j = 1; j <= 2; j++) {
            lua_geti(L, i, j);
            luaL_argcheck(L, lua_isinteger(L, -1), i, "element type must be integer");
            lua_pop(L, 1);
        }
    }
    size_t nranges = nargs - 1;

    size_t list_len = sizeof(HAPUInt8CharacteristicValidValuesRange *) * (nranges + 1);
    HAPUInt8CharacteristicValidValuesRange **list = lua_newuserdatauv(L,
        list_len + sizeof(HAPUInt8CharacteristicValidValuesRange) * nranges, 0);
    lua_setiuservalue(L, 1, 3);
    characteristic->constraints.validValuesRanges =
        (const HAPUInt8CharacteristicValidValuesRange * const *)list;

    HAPUInt8CharacteristicValidValuesRange *ranges =
        (HAPUInt8CharacteristicValidValuesRange *)((uintptr_t)list + list_len);
    for (size_t i = 0; i < nranges; i++) {
        list[i] = ranges + i;
    }
    list[nranges] = NULL;

    for (int i = nranges - 1; i >= 0; i--) {
        lua_geti(L, -1, 1);
        ranges[i].start = lua_tointeger(L, -1);
        lua_pop(L, 1);
        lua_geti(L, -1, 2);
        ranges[i].end = lua_tointeger(L, -1);
        lua_pop(L, 2);
    }

    return 1;
}

static int lhap_start_finish(lua_State *L, int status, lua_KContext extra) {
    lhap_desc *desc = (lhap_desc *)extra;
    desc->co = NULL;
    desc->started = true;
    return 0;
}

static int lhap_accessory_is_valid(lua_State *L) {
    HAPAccessory *acc = luaL_checkudata(L, 1, LHAP_ACCESSORY_NAME);
    bool bridged = lua_toboolean(L, 2);

    lua_pushboolean(L, bridged ? HAPBridgedAccessoryIsValid(acc) : HAPRegularAccessoryIsValid(acc));
    return 1;
}

static int lhap_start(lua_State *L) {
    lhap_desc *desc = &gv_lhap_desc;
    if (desc->started) {
        luaL_error(L, "HAP is already started");
    }

    desc->primary_acc = luaL_checkudata(L, 1, LHAP_ACCESSORY_NAME);
    luaL_argcheck(L, HAPRegularAccessoryIsValid(desc->primary_acc), 1, "invalid primary accessory");
    desc->num_bridged_accs = lhap_optarray(L, 2);
    luaL_checktype(L, 3, LUA_TBOOLEAN);
    bool conf_changed = lua_toboolean(L, 3);
    bool has_session_accept = lhap_optfunction(L, 4);
    bool has_session_invalid = lhap_optfunction(L, 5);

    desc->bridged_accs = NULL;
    if (desc->num_bridged_accs) {
        desc->bridged_accs = lua_newuserdata(L, sizeof(HAPAccessory *) * (desc->num_bridged_accs + 1));
        lua_pushvalue(L, 2);
        lua_setuservalue(L, -2);
        for (size_t i = 1; i <= desc->num_bridged_accs; i++) {
            lua_geti(L, 2, i);
            desc->bridged_accs[i - 1] = luaL_checkudata(L, -1, LHAP_ACCESSORY_NAME);
            if (!HAPBridgedAccessoryIsValid(desc->bridged_accs[i - 1])) {
                luaL_error(L, "bridgedAccessories[%d]: invalid definition", i);
            }
            lua_pop(L, 1);
        }
        desc->bridged_accs[desc->num_bridged_accs] = NULL;
        lua_rawsetp(L, LUA_REGISTRYINDEX, &desc->bridged_accs);
    }

    lua_pushvalue(L, 1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, &desc->primary_acc);

    if (has_session_accept) {
        lua_pushvalue(L, 4);
        lua_rawsetp(L, LUA_REGISTRYINDEX, &desc->server_cbs.handleSessionAccept);
    }
    if (has_session_invalid) {
        lua_pushvalue(L, 5);
        lua_rawsetp(L, LUA_REGISTRYINDEX, &desc->server_cbs.handleSessionInvalidate);
    }

    desc->server_cbs.handleSessionAccept = has_session_accept ? lhap_server_handle_session_accept : NULL;
    desc->server_cbs.handleSessionInvalidate = has_session_invalid ? lhap_server_handle_session_invalid : NULL;
    desc->server_cbs.handleUpdatedState = lhap_server_handle_update_state;

    size_t num_attr = LHAP_ATTR_CNT_DFT;
    size_t num_readable = LHAP_CHAR_READ_CNT_DFT;
    size_t num_writable = LHAP_CHAR_WRITE_CNT_DFT;
    size_t num_notify = LHAP_CHAR_NOTIFY_CNT_DFT;

    lhap_count_attr(desc->primary_acc, &num_attr, &num_readable, &num_writable, &num_notify);

    if (desc->bridged_accs) {
        for (const HAPAccessory * const *pacc =
            (const HAPAccessory * const *)desc->bridged_accs; *pacc; pacc++) {
            lhap_count_attr(*pacc, &num_attr, &num_readable, &num_writable, &num_notify);
        }
    }

    if (num_readable == 0) {
        num_readable = 1;
    }
    if (num_writable == 0) {
        num_writable = 1;
    }
    if (num_notify == 0) {
        num_notify = 1;
    }

    pal_hap_init_platform(&desc->platform);

    // Display setup code.
    HAPSetupCode setupCode;
    HAPPlatformAccessorySetupLoadSetupCode(desc->platform.accessorySetup, &setupCode);
    HAPLog(&lhap_log, "Setup code: %s", setupCode.stringValue);

    lhap_init_ip(&desc->server_options, HAPMax(num_readable, num_writable), num_notify);
    desc->server_options.maxPairings = kHAPPairingStorage_MinElements;

    // Initialize accessory server.
    HAPAccessoryServerCreate(
            &desc->server,
            &desc->server_options,
            &desc->platform,
            &desc->server_cbs,
            desc);

    // Start accessory server.
    if (desc->bridged_accs) {
        HAPAccessoryServerStartBridge(&desc->server, desc->primary_acc,
        (const struct HAPAccessory *const *)desc->bridged_accs, conf_changed);
    } else {
        HAPAccessoryServerStart(&desc->server, desc->primary_acc);
    }

    desc->num_read_requests = 0;
    desc->max_read_requests = LHAP_READ_REQUESTS_MAX;
    desc->read_requests_head = NULL;
    desc->read_requests_ptail = &desc->read_requests_head;

    desc->mL = lc_getmainthread(L);
    desc->co = L;
    return lua_yieldk(L, 0, (lua_KContext)desc, lhap_start_finish);
}

static int lhap_stop_finish(lua_State *L, int status, lua_KContext extra) {
    lhap_desc *desc = (lhap_desc *)extra;

    // Release accessory server.
    HAPAccessoryServerRelease(&desc->server);

    lhap_deinit_ip(&desc->server_options);

    lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &desc->primary_acc);
    lhap_rawsetp_reset(L, LUA_REGISTRYINDEX, &desc->bridged_accs);

    lhap_reset_server_cb(L, &desc->server_cbs);
    HAPRawBufferZero(&desc->server_cbs, sizeof(desc->server_cbs));

    HAPRawBufferZero(&desc->server, sizeof(desc->server));

    desc->primary_acc = NULL;
    desc->bridged_accs = NULL;
    desc->num_bridged_accs = 0;
    desc->co = NULL;
    desc->mL = NULL;
    desc->started = false;
    return 0;
}

static int lhap_stop(lua_State *L) {
    lhap_desc *desc = &gv_lhap_desc;

    if (!desc->started) {
        luaL_error(L, "HAP is not started.");
    }

    // Stop accessory server.
    HAPAccessoryServerStop(&desc->server);

    desc->co = L;
    return lua_yieldk(L, 0, (lua_KContext)desc, lhap_stop_finish);
}

static int lhap_at_exit(lua_State *L) {
    lhap_desc *desc = &gv_lhap_desc;

    if (!desc->started) {
        return 0;
    }
    return lhap_stop(L);
}

static int lhap_raise_event(lua_State *L) {
    HAPSessionRef *session = NULL;
    lhap_desc *desc = &gv_lhap_desc;

    if (!desc->started) {
        luaL_error(L, "HAP is not started.");
    }

    uint64_t aid = luaL_checkinteger(L, 1);
    uint64_t sid = luaL_checkinteger(L, 2);
    uint64_t cid = luaL_checkinteger(L, 3);
    if (!lua_isnoneornil(L, 4)) {
        luaL_checktype(L, 4, LUA_TLIGHTUSERDATA);
        session = lua_touserdata(L, 4);
    }

    HAPAccessoryServerRaiseEventByIID(&desc->server, cid, sid, aid, session);
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

static int lhap_get_setup_code(lua_State *L) {
    lhap_desc *desc = &gv_lhap_desc;

    pal_hap_init_platform(&desc->platform);

    HAPSetupCode setupCode;
    HAPPlatformAccessorySetupLoadSetupCode(desc->platform.accessorySetup, &setupCode);
    lua_pushstring(L, setupCode.stringValue);

    return 1;
}

static int lhap_restore_factory_settings(lua_State *L) {
    lhap_desc *desc = &gv_lhap_desc;

    if (desc->started) {
        luaL_error(L, "HAP is already started");
    }

    if (luai_unlikely(!pal_hap_restore_factory_settings())) {
        luaL_error(L, "failed to restore factory settings");
    }

    pal_nvs_handle *handle = pal_nvs_open(LHAP_NVS_NAMESPACE);
    if (luai_unlikely(!handle)) {
        luaL_error(L, "failed to open '%s'", LHAP_NVS_NAMESPACE);
    }
    if (luai_unlikely(!pal_nvs_erase(handle))) {
        pal_nvs_close(handle);
        luaL_error(L, "failed to erase '%s'", LHAP_NVS_NAMESPACE);
    }
    pal_nvs_close(handle);
    return 0;
}

static const luaL_Reg haplib[] = {
    {"newAccessory", lhap_new_accessory},
    {"newService", lhap_new_service},
    {"newCharacteristic", lhap_new_char},
    {"accessoryIsValid", lhap_accessory_is_valid},
    {"start", lhap_start},
    {"stop", lhap_stop},
    {"raiseEvent", lhap_raise_event},
    {"getNewInstanceID", lhap_get_new_iid},
    {"getSetupCode", lhap_get_setup_code},
    {"restoreFactorySettings", lhap_restore_factory_settings},
    /* placeholders */
    {"AccessoryInformationService", NULL},
    {"HAPProtocolInformationService", NULL},
    {"PairingService", NULL},
    {NULL, NULL},
};

/*
 * metamethods for accessory
 */
static const luaL_Reg lhap_accessory_metameth[] = {
    {"__gc", lhap_accessory_gc},
    {NULL, NULL}
};

/*
 * metamethods for service
 */
static const luaL_Reg lhap_service_metameth[] = {
    {"__index", NULL},  /* place holder */
    {NULL, NULL}
};

/*
 * methods for service
 */
static const luaL_Reg lhap_service_meth[] = {
    {"linkServices", lhap_service_link_services},
    {NULL, NULL},
};

/*
 * metamethods for characteristic
 */
static const luaL_Reg lhap_char_metameth[] = {
    {"__index", NULL},  /* place holder */
    {"__gc", lhap_char_gc},
    {NULL, NULL}
};

/*
 * methods for characteristic
 */
static const luaL_Reg lhap_char_meth[] = {
    {"setMfgDesc", lhap_char_set_mfg_desc},
    {"setUnits", lhap_char_set_units},
    {"setContraints", lhap_char_set_contraints},
    {"setValidVals", lhap_char_set_valid_vals},
    {"setValidValsRanges", lhap_set_valid_vals_ranges},
    {NULL, NULL},
};

static void lhap_createmeta(lua_State *L) {
    luaL_newmetatable(L, LHAP_ACCESSORY_NAME);  /* metatable for accessory */
    luaL_setfuncs(L, lhap_accessory_metameth, 0);  /* add metamethods to new metatable */
    lua_pop(L, 1);  /* pop metatable */

    luaL_newmetatable(L, LHAP_SERVICE_NAME);  /* metatable for service */
    luaL_setfuncs(L, lhap_service_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lhap_service_meth);  /* create method table */
    luaL_setfuncs(L, lhap_service_meth, 0);  /* add service methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */

    luaL_newmetatable(L, LHAP_CHARACTERISTIC_NAME);  /* metatable for characteristic */
    luaL_setfuncs(L, lhap_char_metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, lhap_char_meth);  /* create method table */
    luaL_setfuncs(L, lhap_char_meth, 0);  /* add characteristic methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

LUAMOD_API int luaopen_hap(lua_State *L) {
    luaL_newlib(L, haplib);
    lhap_createmeta(L);

    /* set services */
    for (const lhap_lightuserdata *ud = lhap_accessory_services_userdatas;
        ud->ptr; ud++) {
        lua_pushlightuserdata(L, ud->ptr);
        lua_setfield(L, -2, ud->name);
    }

    lua_getglobal(L, "core");
    lua_getfield(L, -1, "atexit");
    lua_remove(L, -2);
    lua_pushcfunction(L, lhap_at_exit);
    lua_call(L, 1, 0);

    return 1;
}
