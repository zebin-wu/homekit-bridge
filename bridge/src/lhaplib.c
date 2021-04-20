/**
 * Copyright (c) 2021 KNpTrue
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <lualib.h>
#include <lauxlib.h>
#include <HAPCharacteristic.h>
#include "AppInt.h"
#include "lapi.h"
#include "lhaplib.h"
#include "DB.h"

#define LHAP_MALLOC(size)        malloc(size)
#define LHAP_FREE(p)             do { if (p) { free((void *)p); (p) = NULL; } } while (0)

#define LHAP_CASE_CHAR_FORMAT_CODE(format, ptr, code) \
    case kHAPCharacteristicFormat_##format: \
        { HAP##format##Characteristic *ptr = ptr; code; } \
        break;

#define LHAP_LOG_TYPE_ERROR(L, name, excepted, got) \
	do { \
		HAPLogError(&lhap_log, "invalid type: %s", name); \
		HAPLogError(&lhap_log, "%s excepted, got %s", \
		    lua_typename(L, excepted), \
		    lua_typename(L, got)); \
	} while (0)

static const HAPLogObject lhap_log = {
    .subsystem = kHAPApplication_LogSubsystem,
    .category = "lhap",
};

static const char *lhap_lhap_accessory_category_strs[] = {
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

static const char *lhap_error_strs[] = {
    "None",
    "Unknown",
    "InvalidState",
    "InvalidData",
    "OutOfResources",
    "NotAuthorized",
    "Busy",
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
typedef struct {
    const char *name;
    void *ptr;
} lhap_lightuserdata;

static const lhap_lightuserdata lhap_accessory_services_userdatas[] = {
    {"AccessoryInformationService", (void *)&accessoryInformationService},
    {"HapProtocolInformationService", (void *)&hapProtocolInformationService},
    {"PairingService", (void *)&pairingService},
    {NULL, NULL},
};

typedef struct {
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

typedef struct {
    const char *name;
    const HAPUUID *type;
    const char *debugDescription;
    HAPCharacteristicFormat format;
} lhap_characteristic_type;

#define LHAP_CHARACTERISTIC_TYPE_FORMAT(type, format) \
{ \
    #type, \
    &kHAPCharacteristicType_##type, \
    kHAPCharacteristicDebugDescription_##type, \
    kHAPCharacteristicFormat_##format \
}

static const lhap_characteristic_type lhap_characteristic_type_tab[] = {
    LHAP_CHARACTERISTIC_TYPE_FORMAT(AdministratorOnlyAccess, Bool),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(AudioFeedback, Bool),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Brightness, Int),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CoolingThresholdTemperature, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentDoorState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentHeatingCoolingState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentRelativeHumidity, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentTemperature, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(HeatingThresholdTemperature, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Hue, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Identify, Bool),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LockControlPoint, TLV8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LockManagementAutoSecurityTimeout, UInt32),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LockLastKnownAction, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LockCurrentState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LockTargetState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Logs, TLV8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Manufacturer, String),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Model, String),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(MotionDetected, Bool),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Name, String),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ObstructionDetected, Bool),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(On, Bool),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(OutletInUse, Bool),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(RotationDirection, Int),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(RotationSpeed, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Saturation, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SerialNumber, String),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetDoorState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetHeatingCoolingState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetRelativeHumidity, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetTemperature, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TemperatureDisplayUnits, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Version, String),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PairSetup, TLV8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PairVerify, TLV8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PairingFeatures, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PairingPairings, TLV8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(FirmwareRevision, String),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(HardwareRevision, String),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(AirParticulateDensity, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(AirParticulateSize, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SecuritySystemCurrentState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SecuritySystemTargetState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(BatteryLevel, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CarbonMonoxideDetected, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ContactSensorState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentAmbientLightLevel, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentHorizontalTiltAngle, Int),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentPosition, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentVerticalTiltAngle, Int),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(HoldPosition, Bool),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LeakDetected, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(OccupancyDetected, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PositionState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ProgrammableSwitchEvent, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(StatusActive, Bool),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SmokeDetected, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(StatusFault, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(StatusJammed, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(StatusLowBattery, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(StatusTampered, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetHorizontalTiltAngle, Int),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetPosition, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetVerticalTiltAngle, Int),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SecuritySystemAlarmType, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ChargingState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CarbonMonoxideLevel, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CarbonMonoxidePeakLevel, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CarbonDioxideDetected, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CarbonDioxideLevel, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CarbonDioxidePeakLevel, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(AirQuality, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ServiceSignature, Data),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(AccessoryFlags, UInt32),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(LockPhysicalControls, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetAirPurifierState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentAirPurifierState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentSlatState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(FilterLifeLevel, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(FilterChangeIndication, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ResetFilterIndication, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentFanState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(Active, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentHeaterCoolerState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetHeaterCoolerState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentHumidifierDehumidifierState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetHumidifierDehumidifierState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(WaterLevel, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SwingMode, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetFanState, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SlatType, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(CurrentTiltAngle, Int),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(TargetTiltAngle, Int),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(OzoneDensity, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(NitrogenDioxideDensity, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SulphurDioxideDensity, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PM2_5Density, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(PM10Density, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(VOCDensity, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(RelativeHumidityDehumidifierThreshold, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(RelativeHumidityHumidifierThreshold, Float),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ServiceLabelIndex, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ServiceLabelNamespace, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ColorTemperature, UInt32),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ProgramMode, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(InUse, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(SetDuration, UInt32),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(RemainingDuration, UInt32),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ValveType, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(IsConfigured, UInt8),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ActiveIdentifier, UInt32),
    LHAP_CHARACTERISTIC_TYPE_FORMAT(ADKVersion, String),
};

static struct lhap_desc {
    bool isConfigure:1;
    size_t attributeCount;
    HAPAccessory accessory;
    HAPAccessory **bridgedAccessories;
} gv_lhap_desc = {
    .attributeCount = kAttributeCount
};

// Find the string and return the string index.
static int lhap_lookup_by_name(const char *name, const char *strs[], int len)
{
    for (int i = 0; i < len; i++) {
        if (!strcmp(name, strs[i])) {
            return i;
        }
    }
    return -1;
}

// Check if the service is one of "lhap_accessory_services_userdatas"
static bool lhap_service_is_light_userdata(HAPService *service)
{
    for (const lhap_lightuserdata *ud = lhap_accessory_services_userdatas;
        ud->ptr; ud++) {
        if (service == ud->ptr) {
            return true;
        }
    }
    return false;
}

// Return a new string copy from str.
static char *lhap_new_str(const char *str)
{
    if (!str) {
        return NULL;
    }
    char *copy = LHAP_MALLOC(strlen(str) + 1);
    return copy ? strcpy(copy, str) : NULL;
}

static bool
lhap_accessory_aid_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPAccessory *)arg)->aid = lua_tonumber(L, -1);
    return true;
}

static bool
lhap_accessory_category_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    int idx = lhap_lookup_by_name(lua_tostring(L, -1),
        lhap_lhap_accessory_category_strs,
        HAPArrayCount(lhap_lhap_accessory_category_strs));
    if (idx == -1) {
        return false;
    }
    ((HAPAccessory *)arg)->category = idx;
    return true;
}

static bool
lhap_accessory_name_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return (*((char **)&((HAPAccessory *)arg)->name) =
        lhap_new_str(lua_tostring(L, -1))) ? true : false;
}

static bool
lhap_accessory_manufacturer_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return (*((char **)&((HAPAccessory *)arg)->manufacturer) =
        lhap_new_str(lua_tostring(L, -1))) ? true : false;
}

static bool
lhap_accessory_model_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return (*((char **)&((HAPAccessory *)arg)->model) =
        lhap_new_str(lua_tostring(L, -1))) ? true : false;
}

static bool
lhap_accessory_serialnumber_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return (*((char **)&((HAPAccessory *)arg)->serialNumber) =
        lhap_new_str(lua_tostring(L, -1))) ? true : false;
}

static bool
lhap_accessory_firmwareversion_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return (*((char **)&((HAPAccessory *)arg)->firmwareVersion) =
        lhap_new_str(lua_tostring(L, -1))) ? true : false;
}

static bool
lhap_accessory_hardwareversion_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return (*((char **)&((HAPAccessory *)arg)->hardwareVersion) =
        lhap_new_str(lua_tostring(L, -1))) ? true : false;
}

static bool
lhap_service_iid_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPService *)arg)->iid = lua_tonumber(L, -1);
    gv_lhap_desc.attributeCount++;
    return true;
}

static bool
lhap_service_type_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    HAPService *service = arg;
    const char *str = lua_tostring(L, -1);
    for (int i = 0; i < HAPArrayCount(lhap_service_type_tab);
        i++) {
        if (!strcmp(str, lhap_service_type_tab[i].name)) {
            service->serviceType = lhap_service_type_tab[i].type;
            service->debugDescription = lhap_service_type_tab[i].debugDescription;
            return true;
        }
    }
    return false;
}

static bool
lhap_service_name_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return (*((char **)&((HAPService *)arg)->name) =
        lhap_new_str(lua_tostring(L, -1))) ? true : false;
}

static bool
lhap_service_props_primary_service_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPServiceProperties *)arg)->primaryService = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_service_props_hidden_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPServiceProperties *)arg)->hidden = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_service_props_supports_conf_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPServiceProperties *)arg)->ble.supportsConfiguration = lua_toboolean(L, -1);
    return true;
}

static const lapi_table_kv lhap_service_props_ble_kvs[] = {
    {"supportsConfiguration", LUA_TBOOLEAN, lhap_service_props_supports_conf_cb},
    {NULL, LUA_TNONE, NULL},
};

static bool
lhap_service_props_ble_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return lapi_traverse_table(L, -1, lhap_service_props_ble_kvs,
        &((HAPService *)arg)->properties);
}

static const lapi_table_kv lhap_service_props_kvs[] = {
    {"primaryService", LUA_TBOOLEAN, lhap_service_props_primary_service_cb},
    {"hidden", LUA_TBOOLEAN, lhap_service_props_hidden_cb},
    {"ble", LUA_TTABLE, lhap_service_props_ble_cb},
    {NULL, LUA_TNONE, NULL},
};

static bool
lhap_service_props_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return lapi_traverse_table(L, -1, lhap_service_props_kvs,
        &((HAPService *)arg)->properties);
}

static bool
lhap_characteristic_iid_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPBaseCharacteristic *)arg)->iid = lua_tointeger(L, -1);
    return true;
}

static bool
lhap_characteristic_type_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    HAPBaseCharacteristic *c = arg;
    const char *str = lua_tostring(L, -1);
    for (int i = 0; i < HAPArrayCount(lhap_characteristic_type_tab);
        i++) {
        if (!strcmp(str, lhap_characteristic_type_tab[i].name)) {
            if (c->format != lhap_characteristic_type_tab[i].format) {
                HAPLogError(&lhap_log, "%s: Format error, %s expected, got %s",
                    __func__,
                    lhap_characteristic_format_strs[lhap_characteristic_type_tab[i].format],
                    lhap_characteristic_format_strs[c->format]);
                return false;
            }
            c->characteristicType = lhap_characteristic_type_tab[i].type;
            c->debugDescription = lhap_characteristic_type_tab[i].debugDescription;
            return true;
        }
    }
    HAPLogError(&lhap_log, "%s: error type.", __func__);
    return false;
}

static bool
lhap_characteristic_mfg_desc_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return (*((char **)&((HAPBaseCharacteristic *)arg)->manufacturerDescription) =
        lhap_new_str(lua_tostring(L, -1))) ? true : false;
}

static bool
lhap_char_props_readable_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->readable = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_writable_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->writable = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_support_evt_notify_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->supportsEventNotification = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_hidden_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->hidden = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_read_req_admin_pms_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->readRequiresAdminPermissions = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_write_req_admin_pms_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->writeRequiresAdminPermissions = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_req_timed_write_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->requiresTimedWrite = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_support_auth_data_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->supportsAuthorizationData = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_ip_control_point_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->ip.controlPoint = lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_ip_support_write_resp_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->ip.supportsWriteResponse = lua_toboolean(L, -1);
    return true;
}

static const lapi_table_kv lhap_char_props_ip_kvs[] = {
    {"controlPoint", LUA_TBOOLEAN, lhap_char_props_ip_control_point_cb},
    {"supportsWriteResponse", LUA_TBOOLEAN, lhap_char_props_ip_support_write_resp_cb},
    {NULL, LUA_TNONE, NULL},
};

static bool
lhap_char_props_ip_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return lapi_traverse_table(L, -1, lhap_char_props_ip_kvs,
        &((HAPBaseCharacteristic *)arg)->properties);
}

static bool
lhap_char_props_ble_support_bc_notify_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->ble.supportsBroadcastNotification =
        lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_ble_support_disconn_notify_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->ble.supportsDisconnectedNotification =
        lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_ble_read_without_sec_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->ble.readableWithoutSecurity =
        lua_toboolean(L, -1);
    return true;
}

static bool
lhap_char_props_ble_write_without_sec_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPCharacteristicProperties *)arg)->ble.writableWithoutSecurity =
        lua_toboolean(L, -1);
    return true;
}

static const lapi_table_kv lhap_char_props_ble_kvs[] = {
    {
        "supportsBroadcastNotification",
        LUA_TBOOLEAN,
        lhap_char_props_ble_support_bc_notify_cb
    },
    {
        "supportsDisconnectedNotification",
        LUA_TBOOLEAN,
        lhap_char_props_ble_support_disconn_notify_cb
    },
    {
        "readableWithoutSecurity",
        LUA_TBOOLEAN,
        lhap_char_props_ble_read_without_sec_cb
    },
    {
        "writableWithoutSecurity",
        LUA_TBOOLEAN,
        lhap_char_props_ble_write_without_sec_cb
    },
    {NULL, LUA_TNONE, NULL},
};

static bool
lhap_char_props_ble_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return lapi_traverse_table(L, -1, lhap_char_props_ble_kvs,
        &((HAPBaseCharacteristic *)arg)->properties);
}

static const lapi_table_kv lhap_char_props_kvs[] = {
    {"readable", LUA_TBOOLEAN, lhap_char_props_readable_cb},
    {"writable", LUA_TBOOLEAN, lhap_char_props_writable_cb},
    {"supportsEventNotification", LUA_TBOOLEAN, lhap_char_props_support_evt_notify_cb},
    {"hidden", LUA_TBOOLEAN, lhap_char_props_hidden_cb},
    {"readRequiresAdminPermissions", LUA_TBOOLEAN, lhap_char_props_read_req_admin_pms_cb},
    {"writeRequiresAdminPermissions", LUA_TBOOLEAN, lhap_char_props_write_req_admin_pms_cb},
    {"requiresTimedWrite", LUA_TBOOLEAN, lhap_char_props_req_timed_write_cb},
    {"supportsAuthorizationData", LUA_TBOOLEAN, lhap_char_props_support_auth_data_cb},
    {"ip", LUA_TTABLE, lhap_char_props_ip_cb},
    {"ble", LUA_TTABLE, lhap_char_props_ble_cb},
    {NULL, LUA_TNONE, NULL},
};

static bool
lhap_characteristic_properties_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return lapi_traverse_table(L, -1, lhap_char_props_kvs,
        &((HAPBaseCharacteristic *)arg)->properties);
}

static bool
lhap_characteristic_units_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)arg)->format;
    if (format < kHAPCharacteristicFormat_UInt8 ||
        format > kHAPCharacteristicFormat_Float) {
        HAPLogError(&lhap_log, "%s: The value of the %s characteristic has no unit.",
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
    LHAP_CASE_CHAR_FORMAT_CODE(UInt8, arg, arg->units = idx)
    LHAP_CASE_CHAR_FORMAT_CODE(UInt16, arg, arg->units = idx)
    LHAP_CASE_CHAR_FORMAT_CODE(UInt32, arg, arg->units = idx)
    LHAP_CASE_CHAR_FORMAT_CODE(UInt64, arg, arg->units = idx)
    LHAP_CASE_CHAR_FORMAT_CODE(Int, arg, arg->units = idx)
    LHAP_CASE_CHAR_FORMAT_CODE(Float, arg, arg->units = idx)
    default:
        HAPAssertionFailure();
        break;
    }
    return true;
}

static bool
lhap_char_constraints_max_length_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)arg)->format;
    switch (format) {
    LHAP_CASE_CHAR_FORMAT_CODE(String, arg,
        arg->constraints.maxLength = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(Data, arg,
        arg->constraints.maxLength = lua_tointeger(L, -1))
    default:
        HAPLogError(&lhap_log, "%s: The constraints of the %s "
            "characteristic has no maxLength.",
            __func__, lhap_characteristic_format_strs[format]);
        return false;
    }
    return true;
}

static bool
lhap_char_constraints_min_val_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)arg)->format;
    switch (format) {
    LHAP_CASE_CHAR_FORMAT_CODE(UInt8, arg,
        arg->constraints.minimumValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(UInt16, arg,
        arg->constraints.minimumValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(UInt32, arg,
        arg->constraints.minimumValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(UInt64, arg,
        arg->constraints.minimumValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(Int, arg,
        arg->constraints.minimumValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(Float, arg,
        arg->constraints.minimumValue = lua_tonumber(L, -1))
    default:
        HAPLogError(&lhap_log, "%s: The constraints of the %s "
            "characteristic has no minimumValue.",
            __func__, lhap_characteristic_format_strs[format]);
        return false;
    }
    return true;
}

static bool
lhap_char_constraints_max_val_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)arg)->format;
    switch (format) {
    LHAP_CASE_CHAR_FORMAT_CODE(UInt8, arg,
        arg->constraints.maximumValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(UInt16, arg,
        arg->constraints.maximumValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(UInt32, arg,
        arg->constraints.maximumValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(UInt64, arg,
        arg->constraints.maximumValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(Int, arg,
        arg->constraints.maximumValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(Float, arg,
        arg->constraints.maximumValue = lua_tonumber(L, -1))
    default:
        HAPLogError(&lhap_log, "%s: The constraints of the %s "
            "characteristic has no maximumValue.",
            __func__, lhap_characteristic_format_strs[format]);
        return false;
    }
    return true;
}

static bool
lhap_char_constraints_step_val_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)arg)->format;
    switch (format) {
    LHAP_CASE_CHAR_FORMAT_CODE(UInt8, arg,
        arg->constraints.stepValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(UInt16, arg,
        arg->constraints.stepValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(UInt32, arg,
        arg->constraints.stepValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(UInt64, arg,
        arg->constraints.stepValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(Int, arg,
        arg->constraints.stepValue = lua_tointeger(L, -1))
    LHAP_CASE_CHAR_FORMAT_CODE(Float, arg,
        arg->constraints.stepValue = lua_tonumber(L, -1))
    default:
        HAPLogError(&lhap_log, "%s: The constraints of the %s "
            "characteristic has no stepValue.",
            __func__, lhap_characteristic_format_strs[format]);
        return false;
    }
    return true;
}

bool lhap_char_constraints_valid_vals_arr_cb(lua_State *L, int i, void *arg)
{
    uint8_t **vals = arg;

    if (!lua_isinteger(L, -1)) {
        LHAP_LOG_TYPE_ERROR(L, "element of validValues",
            LUA_TNUMBER, lua_type(L, -1));
        return false;
    }
    uint8_t *val = LHAP_MALLOC(sizeof(uint8_t));
    if (!val) {
        HAPLogError(&lhap_log, "%s: Failed to alloc.", __func__);
        return false;
    }
    *val = lua_tointeger(L, -1);
    vals[i] = val;
    return true;
}

static bool
lhap_char_constraints_valid_vals_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)arg)->format;
    switch (format) {
    LHAP_CASE_CHAR_FORMAT_CODE(UInt8, arg,
        uint8_t ***pValidValues = (uint8_t ***)&(arg->constraints.validValues);
        lua_Unsigned len = lua_rawlen(L, -1);
        if (!len) {
            *pValidValues = NULL;
            break;
        }
        uint8_t **vals = LHAP_MALLOC(sizeof(uint8_t *) * len);
        if (!vals) {
            HAPLogError(&lhap_log, "%s: Failed to alloc.", __func__);
            return false;
        }
        memset(vals, 0, sizeof(uint8_t *) * len);
        if (!lapi_traverse_array(L, -1, lhap_char_constraints_valid_vals_arr_cb,
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
lhap_char_constraints_valid_val_range_start_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPUInt8CharacteristicValidValuesRange *)arg)->start = lua_tointeger(L, -1);
    return true;
}

static bool
lhap_char_constraints_valid_val_range_end_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    ((HAPUInt8CharacteristicValidValuesRange *)arg)->end = lua_tointeger(L, -1);
    return true;
}

static const lapi_table_kv lhap_char_constraints_valid_val_range_kvs[] = {
    {"start", LUA_TNUMBER, lhap_char_constraints_valid_val_range_start_cb},
    {"end", LUA_TNUMBER, lhap_char_constraints_valid_val_range_end_cb},
    {NULL, LUA_TNONE, NULL},
};

bool lhap_char_constraints_valid_vals_ranges_arr_cb(lua_State *L, int i, void *arg)
{
    HAPUInt8CharacteristicValidValuesRange **ranges = arg;

    if (!lua_istable(L, -1)) {
        LHAP_LOG_TYPE_ERROR(L, "element of validValuesRanges",
            LUA_TTABLE, lua_type(L, -1));
        return false;
    }
    HAPUInt8CharacteristicValidValuesRange *range =
        LHAP_MALLOC(sizeof(HAPUInt8CharacteristicValidValuesRange));
    if (!range) {
        HAPLogError(&lhap_log, "%s: Failed to alloc.", __func__);
        return false;
    }
    if (!lapi_traverse_table(L, -1, lhap_char_constraints_valid_val_range_kvs, range)) {
        HAPLogError(&lhap_log, "%s: Failed to parse valid value range.", __func__);
        LHAP_FREE(range);
        return false;
    }
    ranges[i] = range; 
    return true;
}

static bool
lhap_char_constraints_valid_vals_ranges_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)arg)->format;
    switch (format) {
    LHAP_CASE_CHAR_FORMAT_CODE(UInt8, arg,
        HAPUInt8CharacteristicValidValuesRange ***pValidValuesRanges =
            (HAPUInt8CharacteristicValidValuesRange ***)
            &(arg->constraints.validValuesRanges);
        lua_Unsigned len = lua_rawlen(L, -1);
        if (!len) {
            *pValidValuesRanges = NULL;
            break;
        }
        HAPUInt8CharacteristicValidValuesRange **ranges =
            LHAP_MALLOC(sizeof(HAPUInt8CharacteristicValidValuesRange *) * len);
        if (!ranges) {
            HAPLogError(&lhap_log, "%s: Failed to alloc ranges.", __func__);
            return false;
        }
        memset(ranges, 0, sizeof(HAPUInt8CharacteristicValidValuesRange *) * len);
        if (!lapi_traverse_array(L, -1, lhap_char_constraints_valid_vals_ranges_arr_cb,
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

static const lapi_table_kv lhap_characteristic_constraints_kvs[] = {
    {"maxLength", LUA_TNUMBER, lhap_char_constraints_max_length_cb},
    {"minimumValue", LUA_TNUMBER, lhap_char_constraints_min_val_cb},
    {"maximumValue", LUA_TNUMBER, lhap_char_constraints_max_val_cb},
    {"stepValue", LUA_TNUMBER, lhap_char_constraints_step_val_cb},
    {"validValues", LUA_TTABLE, lhap_char_constraints_valid_vals_cb},
    {"validValuesRanges", LUA_TTABLE, lhap_char_constraints_valid_vals_ranges_cb},
    {NULL, LUA_TNONE, NULL},
};

static bool
lhap_characteristic_constraints_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    HAPCharacteristicFormat format = ((HAPBaseCharacteristic *)arg)->format;
    if (format == kHAPCharacteristicFormat_Bool ||
        format == kHAPCharacteristicFormat_TLV8) {
        HAPLogError(&lhap_log, "%s: The %s characteristic has no constraints.",
            __func__, lhap_characteristic_format_strs[format]);
        return false;
    }
    return lapi_traverse_table(L, -1, lhap_characteristic_constraints_kvs, arg);
}

static bool
lhap_char_callbacks_handle_read_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return false;
}

static bool
lhap_char_callbacks_handle_write_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return false;
}

static bool
lhap_char_callbacks_handle_sub_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return false;
}

static bool
lhap_char_callbacks_handle_unsub_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return false;
}

static const lapi_table_kv lhap_characteristic_callbacks_kvs[] = {
    {"handleRead", LUA_TFUNCTION, lhap_char_callbacks_handle_read_cb},
    {"handleWrite", LUA_TFUNCTION, lhap_char_callbacks_handle_write_cb},
    {"handleSubscribe", LUA_TFUNCTION, lhap_char_callbacks_handle_sub_cb},
    {"handleUnsubscribe", LUA_TFUNCTION, lhap_char_callbacks_handle_unsub_cb},
    {NULL, LUA_TNONE, NULL},
};

static bool
lhap_characteristic_callbacks_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return lapi_traverse_table(L, -1, lhap_characteristic_callbacks_kvs, arg);
}

static const lapi_table_kv lhap_characteristic_kvs[] = {
    {"iid", LUA_TNUMBER, lhap_characteristic_iid_cb},
    {"type", LUA_TSTRING, lhap_characteristic_type_cb},
    {"manufacturerDescription", LUA_TSTRING, lhap_characteristic_mfg_desc_cb},
    {"properties", LUA_TTABLE, lhap_characteristic_properties_cb},
    {"units", LUA_TSTRING, lhap_characteristic_units_cb},
    {"constraints", LUA_TTABLE, lhap_characteristic_constraints_cb},
    {"callbacks", LUA_TTABLE, lhap_characteristic_callbacks_cb},
    {NULL, LUA_TNONE, NULL},
};

static bool lhap_service_characteristics_arr_cb(lua_State *L, int i, void *arg)
{
    HAPCharacteristic **characteristics = arg;

    if (!lua_istable(L, -1)) {
        return false;
    }

    lua_pushstring(L, "format");
    lua_gettable(L, -2);
    if (!lua_isstring(L, -1)) {
        return false;
    }
    int idx = lhap_lookup_by_name(lua_tostring(L, -1),
        lhap_characteristic_format_strs, 
        HAPArrayCount(lhap_characteristic_format_strs));
    lua_pop(L, 1);
    if (idx == -1) {
        return false;
    }

    HAPCharacteristic *c = LHAP_MALLOC(lhap_characteristic_struct_size[idx]);
    if (!c) {
        return false;
    }
    memset(c, 0, sizeof(HAPCharacteristic));
    ((HAPBaseCharacteristic *)c)->format = idx;
    characteristics[i] = c;
    if (!lapi_traverse_table(L, -1, lhap_characteristic_kvs, c)) {
        return false;
    }
    return true;
}

static void lhap_reset_characteristic(HAPCharacteristic *characteristic)
{
    LHAP_FREE(((HAPBaseCharacteristic *)characteristic)->manufacturerDescription);
}

static bool
lhap_service_characteristics_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    HAPService *service = arg;
    HAPCharacteristic ***pcharacteristic = (HAPCharacteristic ***)&(service->characteristics);
    // Get the array length.
    lua_Unsigned len = lua_rawlen(L, -1);
    if (len == 0) {
        *pcharacteristic = NULL;
        return true;
    }

    HAPCharacteristic **characteristics = LHAP_MALLOC(sizeof(HAPCharacteristic *) * (len + 1));
    if (!characteristics) {
        return false;
    }
    if (!lapi_traverse_array(L, -1, lhap_service_characteristics_arr_cb, characteristics)) {
        for (HAPCharacteristic **c = characteristics; *c; c++) {
            lhap_reset_characteristic(*c);
            LHAP_FREE(*c);
        }
        LHAP_FREE(characteristics);
        return false;
    }
    *pcharacteristic = characteristics;
    return true;
}

static const lapi_table_kv lhap_service_kvs[] = {
    {"iid", LUA_TNUMBER, lhap_service_iid_cb},
    {"type", LUA_TSTRING, lhap_service_type_cb},
    {"name", LUA_TSTRING, lhap_service_name_cb},
    {"properties", LUA_TTABLE, lhap_service_props_cb},
    {"characteristics", LUA_TTABLE, lhap_service_characteristics_cb},
    {NULL, LUA_TNONE, NULL},
};

static void lhap_reset_service(HAPService *service)
{
    LHAP_FREE(service->name);
    for (HAPCharacteristic **c = (HAPCharacteristic **)service->characteristics; *c; c++) {
        lhap_reset_characteristic(*c);
        LHAP_FREE(*c);
    }
    LHAP_FREE(service->characteristics);
}

static bool
lhap_accessory_services_arr_cb(lua_State *L, int i, void *arg)
{
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
    HAPService *s = LHAP_MALLOC(sizeof(HAPService));
    if (!s) {
        return false;
    }
    memset(s, 0, sizeof(HAPService));
    services[i] = s;
    if (!lapi_traverse_table(L, -1, lhap_service_kvs, s)) {
        return false;
    }
    return true;
}

static bool
lhap_accessory_services_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    HAPAccessory *accessory = arg;
    HAPService ***pservices = (HAPService ***)&(accessory->services);

    // Get the array length.
    lua_Unsigned len = lua_rawlen(L, -1);
    if (len == 0) {
        *pservices = NULL;
        return true;
    }

    HAPService **services = LHAP_MALLOC(sizeof(HAPService *) * (len + 1));
    if (!services) {
        HAPLogError(&lhap_log,
            "%s: Failed to alloc memory.", __func__);
        return false;
    }
    memset(services, 0, sizeof(HAPService *) * (len + 1));

    if (!lapi_traverse_array(L, -1, lhap_accessory_services_arr_cb, services)) {
        for (HAPService **s = services; *s; s++) {
            if (!lhap_service_is_light_userdata(*s)) {
                lhap_reset_service(*s);
                LHAP_FREE(*s);
            }
        }
        LHAP_FREE(services);
        return false;
    }

    *pservices = services;
    return true;
}

HAP_RESULT_USE_CHECK
HAPError lhap_accessory_identify_cb(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPAccessoryIdentifyRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(context);

    HAPError err = kHAPError_Unknown;
    lua_State *L = ((AccessoryContext *)context)->L;

    if (!lapi_push_callback(L,
        (size_t)&(request->accessory->callbacks.identify))) {
        HAPLogError(&lhap_log,
            "%s: Can't get lua function.", __func__);
        return err;
    }

    if (lua_pcall(L, 0, 1, 0)) {
        HAPLogError(&lhap_log,
            "%s: Failed to call lua function.", __func__);
        return err;
    }

    if (!lua_isnumber(L, -1)) {
        HAPLogError(&lhap_log,
            "%s: Illegal return value.", __func__);
        goto end;
    }

    err = lua_tonumber(L, -1);
end:
    lua_pop(L, 1);
    lapi_collectgarbage(L);
    return err;
}

static bool
lhap_accessory_cbs_identify_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    if (!lapi_register_callback(L, -1,
        (size_t)&(((HAPAccessory *)arg)->callbacks.identify))) {
        HAPLogError(&lhap_log,
            "%s: Failed to register callback.", __func__);
        return false;
    }
    ((HAPAccessory *)arg)->callbacks.identify = lhap_accessory_identify_cb;
    return true;
}

static lapi_table_kv lhap_accessory_callbacks_kvs[] = {
    {"identify", LUA_TFUNCTION, lhap_accessory_cbs_identify_cb},
    {NULL, -1, NULL},
};

static bool
lhap_accessory_callbacks_cb(lua_State *L, const lapi_table_kv *kv, void *arg)
{
    return lapi_traverse_table(L, -1, lhap_accessory_callbacks_kvs, arg);
}

static const lapi_table_kv lhap_accessory_kvs[] = {
    {"aid", LUA_TNUMBER, lhap_accessory_aid_cb},
    {"category", LUA_TSTRING, lhap_accessory_category_cb},
    {"name", LUA_TSTRING, lhap_accessory_name_cb},
    {"manufacturer", LUA_TSTRING, lhap_accessory_manufacturer_cb},
    {"model", LUA_TSTRING, lhap_accessory_model_cb},
    {"serialNumber", LUA_TSTRING, lhap_accessory_serialnumber_cb},
    {"firmwareVersion", LUA_TSTRING, lhap_accessory_firmwareversion_cb},
    {"hardwareVersion", LUA_TSTRING, lhap_accessory_hardwareversion_cb},
    {"services", LUA_TTABLE, lhap_accessory_services_cb},
    {"callbacks", LUA_TTABLE, lhap_accessory_callbacks_cb},
    {NULL, LUA_TNONE, NULL},
};

static void lhap_reset_accessory(HAPAccessory *accessory)
{
    LHAP_FREE(accessory->name);
    LHAP_FREE(accessory->manufacturer);
    LHAP_FREE(accessory->model);
    LHAP_FREE(accessory->serialNumber);
    LHAP_FREE(accessory->firmwareVersion);
    LHAP_FREE(accessory->hardwareVersion);
    for (HAPService **s = (HAPService **)accessory->services; *s; s++) {
        if (!lhap_service_is_light_userdata(*s)) {
            lhap_reset_service(*s);
            LHAP_FREE(*s);
        }
    }
    LHAP_FREE(accessory->services);
}

bool lhap_accessories_arr_cb(lua_State *L, int i, void *arg)
{
    HAPAccessory **accessories = arg;
    if (!lua_istable(L, -1)) {
        HAPLogError(&lhap_log,
            "%s: The type of the element is not table.", __func__);
        return false;
    }
    HAPAccessory *a = LHAP_MALLOC(sizeof(HAPAccessory));
    if (!a) {
        HAPLogError(&lhap_log,
            "%s: Failed to alloc memory.", __func__);
        return false;
    }
    memset(a, 0, sizeof(HAPAccessory));
    accessories[i] = a;
    if (!lapi_traverse_table(L, -1, lhap_accessory_kvs, a)) {
        HAPLogError(&lhap_log,
            "%s: Failed to generate accessory structure from table accessory.",
            __func__);
        return false;
    }
    return true;
}

/**
 * configure(accessory: table, bridgedAccessories?: table) -> boolean
 *
 * If the category of the accessory is bridge, the parameters
 * bridgedAccessories is valid.
*/
static int hap_configure(lua_State *L)
{
    lua_Unsigned len = 0;
    struct lhap_desc *desc = &gv_lhap_desc;
    HAPAccessory *accessory = &desc->accessory;

    if (desc->isConfigure) {
        HAPLogError(&lhap_log,
            "%s: HAP is already configured.", __func__);
        goto err;
    }

    luaL_checktype(L, 1, LUA_TTABLE);
    if (!lua_isnone(L, 2)) {
        luaL_checktype(L, 2, LUA_TTABLE);
    }

    if (!lapi_traverse_table(L, 1, lhap_accessory_kvs, accessory)) {
        HAPLogError(&lhap_log,
            "%s: Failed to generate accessory structure from table accessory.",
            __func__);
        goto err1;
    }

    if (accessory->category != kHAPAccessoryCategory_Bridges) {
        goto end;
    }

    len = lua_rawlen(L, 2);
    if (len == 0) {
        goto end;
    }

    desc->bridgedAccessories =
        LHAP_MALLOC(sizeof(HAPAccessory *) * (len + 1));
    if (!desc->bridgedAccessories) {
        HAPLogError(&lhap_log,
            "%s: Error type(bridgedAccessories).", __func__);
        goto err1;
    }
    memset(desc->bridgedAccessories, 0,
        sizeof(HAPAccessory *) * (len + 1));

    if (!lapi_traverse_array(L, 2, lhap_accessories_arr_cb,
        desc->bridgedAccessories)) {
        HAPLogError(&lhap_log,
            "%s: Failed to generate bridged accessories structureies"
            " from table bridgedAccessories.", __func__);
        goto err2;
    }
end:
    HAPLogInfo(&lhap_log,
        "Accessory \"%s\": %s has been configured.", accessory->name,
        lhap_lhap_accessory_category_strs[accessory->category]);
    if (len) {
        HAPLogInfo(&lhap_log,
            "%llu bridged accessories have been configured.", len);
    }
    desc->isConfigure = true;
    lua_pushboolean(L, true);
    return 1;

err2:
    for (HAPAccessory **pa = desc->bridgedAccessories; *pa != NULL; pa++) {
        lhap_reset_accessory(*pa);
        LHAP_FREE(*pa);
    }
    LHAP_FREE(desc->bridgedAccessories);
err1:
    lhap_reset_accessory(accessory);
err:
    lua_pushboolean(L, false);
    return 1;
}

static const luaL_Reg haplib[] = {
    {"configure", hap_configure},
    /* placeholders */
    {"Error", NULL},
    {"AccessoryInformationService", NULL},
    {"HapProtocolInformationService", NULL},
    {"PairingService", NULL},
    {NULL, NULL},
};

LUAMOD_API int luaopen_hap(lua_State *L) {
    luaL_newlib(L, haplib);

    /* set Error */
    lapi_create_enum_table(L, lhap_error_strs,
        HAPArrayCount(lhap_error_strs));
    lua_setfield(L, -2, "Error");

    /* set services */
    for (const lhap_lightuserdata *ud = lhap_accessory_services_userdatas;
        ud->ptr; ud++) {
        lua_pushlightuserdata(L, ud->ptr);
        lua_setfield(L, -2, ud->name);
    }
    return 1;
}

const HAPAccessory *lhap_get_accessory(void)
{
    if (gv_lhap_desc.isConfigure) {
        return &gv_lhap_desc.accessory;
    }
    return NULL;
}

const HAPAccessory *const *lhap_get_bridged_accessories(void)
{
    if (gv_lhap_desc.isConfigure) {
        return (const HAPAccessory *const *)gv_lhap_desc.bridgedAccessories;
    }
    return NULL;
}

size_t lhap_get_attribute_count(void)
{
    if (gv_lhap_desc.isConfigure) {
        return gv_lhap_desc.attributeCount;
    }
    return 0;
}

void lhap_deinitialize(void)
{
    struct lhap_desc *desc = &gv_lhap_desc;
    for (HAPAccessory **pa = desc->bridgedAccessories; *pa != NULL; pa++) {
        lhap_reset_accessory(*pa);
        LHAP_FREE(*pa);
    }
    LHAP_FREE(desc->bridgedAccessories);
    lhap_reset_accessory(&desc->accessory);
    desc->attributeCount = kAttributeCount;
    desc->isConfigure = false;
}
