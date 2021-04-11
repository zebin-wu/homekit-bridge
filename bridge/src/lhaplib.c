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

/**
 * accessory: {
 *     aid: number, // Accessory instance ID.
 *     category: number,  // Category information for the accessory.
 *     name: string,  // The display name of the accessory.
 *     manufacturer: string,  // The manufacturer of the accessory.
 *     model: string, // The model name of the accessory.
 *     serialNumber: string, // The serial number of the accessory.
 *     firmwareVersion: string, // The firmware version of the accessory.
 *     hardwareVersion: string, // The hardware version of the accessory.
 *     services: table // Array of provided services.
 *     callbacks: {
 *         // The callback used to invoke the identify routine.
 *         identify: function() -> int,
 *     } // Callbacks.
 * } // HomeKit accessory.
 *
 * service: {
 *     iid: number, // Instance ID.
 *     serviceType: number, // The type of the service.
 *     debugDescription: string, // Description for debugging.
 *     name: string, // The name of the service.
 *     properties: {
 *         // The service is the primary service on the accessory.
 *         primaryService: boolean,
 *         hidden: boolean, // The service should be hidden from the user.
 *         ble: {
 *             // The service supports configuration.
 *             supportsConfiguration: boolean,
 *         } These properties only affect connections over Bluetooth LE.
 *     } // HAP Service properties.
 *     characteristics: table, // Array of contained characteristics.
 * } // HomeKit service.
*/

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <common/lapi.h>
#include <lualib.h>
#include <lauxlib.h>

#include "AppInt.h"
#include "lhaplib.h"
#include "DB.h"

#define LHAP_ARRAY_LEN(x)        (sizeof(x) / sizeof(*(x)))
#define LHAP_MALLOC(size)        malloc(size)
#define LHAP_FREE(p)             do { if (p) { free((void *)p); (p) = NULL; } } while (0)

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

static const char *lhap_error_strs[] = {
    "None",
    "Unknown",
    "InvalidState",
    "InvalidData",
    "OutOfResources",
    "NotAuthorized",
    "Busy",
};

/**
 * Function type.
*/
enum func_type {
    FUNC_TYPE_IDENTIFY,
};

static lapi_userdata userdataServices[] = {
    {"AccessoryInformationService", (void *)&accessoryInformationService},
    {"HapProtocolInformationService", (void *)&hapProtocolInformationService},
    {"PairingService", (void *)&pairingService},
    {NULL, NULL},
};

static struct hap_desc {
    bool isConfigure:1;
    size_t attributeCount;
    HAPAccessory accessory;
    HAPAccessory **bridgedAccessories;
    lapi_callback *cbHead;
} gv_hap_desc = {
    .attributeCount = kAttributeCount
};

static bool lhap_check_is_valid_service(HAPService *service)
{
    return lapi_check_is_valid_userdata(userdataServices, service);
}

/* return a new string copy from str */
static char *lhap_new_str(const char *str)
{
    if (!str) {
        return NULL;
    }
    char *copy = LHAP_MALLOC(strlen(str) + 1);
    return copy ? strcpy(copy, str) : NULL;
}

static bool
accessory_aid_cb(lua_State *L, lapi_table_kv *kv, void *arg)
{
    ((HAPAccessory *)arg)->aid = lua_tonumber(L, -1);
    return true;
}

static bool
accessory_category_cb(lua_State *L, lapi_table_kv *kv, void *arg)
{
    ((HAPAccessory *)arg)->category = lua_tonumber(L, -1);
    return true;
}

static bool
accessory_name_cb(lua_State *L, lapi_table_kv *kv, void *arg)
{
    char **name = (char **)&(((HAPAccessory *)arg)->name);
    return (*name = lhap_new_str(lua_tostring(L, -1))) ?
        true : false;
}

static bool
accessory_manufacturer_cb(lua_State *L, lapi_table_kv *kv, void *arg)
{
    char **manufacturer = (char **)&(((HAPAccessory *)arg)->manufacturer);
    return (*manufacturer = lhap_new_str(lua_tostring(L, -1))) ?
        true : false;
}

static bool
accessory_model_cb(lua_State *L, lapi_table_kv *kv, void *arg)
{
    char **model = (char **)&(((HAPAccessory *)arg)->model);
    return (*model = lhap_new_str(lua_tostring(L, -1))) ?
        true : false;
}

static bool
accessory_serialnumber_cb(lua_State *L, lapi_table_kv *kv, void *arg)
{
    char **serialNumber = (char **)&(((HAPAccessory *)arg)->serialNumber);
    return (*serialNumber = lhap_new_str(lua_tostring(L, -1))) ?
        true : false;
}

static bool
accessory_firmwareversion_cb(lua_State *L, lapi_table_kv *kv, void *arg)
{
    char **firmwareVersion = (char **)&(((HAPAccessory *)arg)->firmwareVersion);
    return (*firmwareVersion = lhap_new_str(lua_tostring(L, -1))) ?
        true : false;
}

static bool
accessory_hardwareversion_cb(lua_State *L, lapi_table_kv *kv, void *arg)
{
    char **hardwareVersion = (char **)&(((HAPAccessory *)arg)->hardwareVersion);
    return (*hardwareVersion = lhap_new_str(lua_tostring(L, -1))) ?
        true : false;
}

static lapi_table_kv service_kvs[] = {
    NULL,
};

static void reset_service(HAPService *service)
{
}

static bool
accessory_services_arr_cb(lua_State *L, int i, void *arg)
{
    HAPService **services = arg;

    if (lua_islightuserdata(L, -1)) {
        HAPService *s = lua_touserdata(L, -1);
        if (lhap_check_is_valid_service(s)) {
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
    if (!lapi_traverse_table(L, -1, service_kvs, s)) {
        return false;
    }
    return true;
}

static bool
accessory_services_cb(lua_State *L, lapi_table_kv *kv, void *arg)
{
    HAPAccessory *accessory = arg;
    HAPService ***pservices = (HAPService ***)&(accessory->services);

    // Get the array length.
    size_t len = lua_rawlen(L, -1);
    if (len == 0) {
        *pservices = NULL;
        return true;
    }

    HAPService **services = LHAP_MALLOC(sizeof(HAPService *) * (len + 1));
    if (!services) {
        goto err;
    }
    memset(services, 0, sizeof(HAPService *) * (len + 1));

    if (!lapi_traverse_array(L, -1, accessory_services_arr_cb, services)) {
        goto err1;
    }

    *pservices = services;
    return true;
err1:
    for (HAPService **s = services; *s != NULL; s++) {
        if (!lhap_check_is_valid_service(*s)) {
            reset_service(*s);
            LHAP_FREE(*s);
        }
    }
    LHAP_FREE(services);
err:
    return false;
}

HAP_RESULT_USE_CHECK
HAPError accessory_identify_cb(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPAccessoryIdentifyRequest* request,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(request);
    HAPPrecondition(context);

    HAPError err = kHAPError_Unknown;
    lua_State *L = ((AccessoryContext *)context)->L;

    if (!lapi_push_callback(gv_hap_desc.cbHead, L,
        (void *)&(request->accessory->callbacks.identify),
        FUNC_TYPE_IDENTIFY)) {
        HAPLogError(&kHAPLog_Default,
            "%s: Can't get lua function", __func__);
        return err;
    }

    if (lua_pcall(L, 0, 1, 0)) {
        HAPLogError(&kHAPLog_Default,
            "%s: Failed to call lua function", __func__);
        return err;
    }

    if (!lua_isnumber(L, -1)) {
        HAPLogError(&kHAPLog_Default,
            "%s: Illegal return value", __func__);
        goto end;
    }

    err = lua_tonumber(L, -1);
end:
    lua_pop(L, 1);
    return err;
}

static bool
accessory_cbs_identify_cb(lua_State *L, lapi_table_kv *kv, void *arg)
{
    if (!lapi_register_callback(&gv_hap_desc.cbHead, L, -1,
        &(((HAPAccessory *)arg)->callbacks.identify),
        FUNC_TYPE_IDENTIFY)) {
        return false;
    }
    ((HAPAccessory *)arg)->callbacks.identify = accessory_identify_cb;
    return true;
}

static lapi_table_kv accessory_callbacks_kvs[] = {
    {"identify", LUA_TFUNCTION, accessory_cbs_identify_cb},
    {NULL, -1, NULL},
};

static bool
accessory_callbacks_cb(lua_State *L, lapi_table_kv *kv, void *arg)
{
    return lapi_traverse_table(L, -1, accessory_callbacks_kvs, arg);
}

static lapi_table_kv accessory_kvs[] = {
    {"aid", LUA_TNUMBER, accessory_aid_cb},
    {"category", LUA_TNUMBER, accessory_category_cb},
    {"name", LUA_TSTRING, accessory_name_cb},
    {"manufacturer", LUA_TSTRING, accessory_manufacturer_cb},
    {"model", LUA_TSTRING, accessory_model_cb},
    {"serialNumber", LUA_TSTRING, accessory_serialnumber_cb},
    {"firmwareVersion", LUA_TSTRING, accessory_firmwareversion_cb},
    {"hardwareVersion", LUA_TSTRING, accessory_hardwareversion_cb},
    {"services", LUA_TTABLE, accessory_services_cb},
    {"callbacks", LUA_TTABLE, accessory_callbacks_cb},
    {NULL, -1, NULL},
};

static void reset_accessory(HAPAccessory *accessory)
{
    LHAP_FREE(accessory->name);
    LHAP_FREE(accessory->manufacturer);
    LHAP_FREE(accessory->model);
    LHAP_FREE(accessory->serialNumber);
    LHAP_FREE(accessory->firmwareVersion);
    LHAP_FREE(accessory->hardwareVersion);
}

bool accessories_arr_cb(lua_State *L, int i, void *arg)
{
    HAPAccessory **accessories = arg;
    if (!lua_istable(L, -1)) {
        return false;
    }
    HAPAccessory *a = LHAP_MALLOC(sizeof(HAPAccessory));
    if (!a) {
        return false;
    }
    memset(a, 0, sizeof(HAPAccessory));
    accessories[i] = a;
    if (!lapi_traverse_table(L, -1, accessory_kvs, a)) {
        return false;
    }
    return true;
}

/**
 * configure(accessory: table, bridgedAccessories: table) -> boolean
 *
 * If the category of the accessory is bridge, the parameters
 * bridgedAccessories is valid.
*/
static int hap_configure(lua_State *L)
{
    size_t len;
    struct hap_desc *desc = &gv_hap_desc;
    HAPAccessory *accessory = &desc->accessory;

    if (desc->isConfigure) {
        goto err;
    }

    if (!lua_istable(L, 1)) {
        goto err;
    }

    if (!lapi_traverse_table(L, 1, accessory_kvs, accessory)) {
        HAPLogError(&kHAPLog_Default,
            "%s: Failed to generate accessory structure from table accessory.",
            __func__);
        goto err1;
    }

    if (accessory->category != kHAPAccessoryCategory_Bridges) {
        goto end;
    }

    if (!lua_istable(L, 2)) {
        goto err1;
    }

    len = lua_rawlen(L, 2);
    if (len == 0) {
        goto end;
    }

    desc->bridgedAccessories =
        LHAP_MALLOC(sizeof(HAPAccessory *) * (len + 1));
    if (!desc->bridgedAccessories) {
        goto err1;
    }
    memset(desc->bridgedAccessories, 0,
        sizeof(HAPAccessory *) * (len + 1));

    if (!lapi_traverse_array(L, 2, accessories_arr_cb,
        desc->bridgedAccessories)) {
        goto err2;
    }
end:
    desc->isConfigure = true;
    lua_pushboolean(L, true);
    return 1;

err2:
    for (HAPAccessory **pa = desc->bridgedAccessories; *pa != NULL; pa++) {
        reset_accessory(*pa);
        LHAP_FREE(*pa);
    }
    LHAP_FREE(desc->bridgedAccessories);
err1:
    reset_accessory(accessory);
err:
    lua_pushboolean(L, false);
    return 1;
}

static const luaL_Reg haplib[] = {
    {"configure", hap_configure},
    /* placeholders */
    {"AccessoryCategory", NULL},
    {"Error", NULL},
    {"AccessoryInformationService", NULL},
    {"HapProtocolInformationService", NULL},
    {"PairingService", NULL},
    {NULL, NULL},
};

LUAMOD_API int luaopen_hap(lua_State *L) {
    luaL_newlib(L, haplib);

    /* set AccessoryCategory */
    lapi_create_enum_table(L, lhap_accessory_category_strs,
        LHAP_ARRAY_LEN(lhap_accessory_category_strs));
    lua_setfield(L, -2, "AccessoryCategory");

    /* set Error */
    lapi_create_enum_table(L, lhap_error_strs,
        LHAP_ARRAY_LEN(lhap_error_strs));
    lua_setfield(L, -2, "Error");

    /* set services */
    for (lapi_userdata *ud = userdataServices; ud->userdata; ud++) {
        lua_pushlightuserdata(L, ud->userdata);
        lua_setfield(L, -2, ud->name);
    }
    return 1;
}

const HAPAccessory *lhap_get_accessory(void)
{
    if (gv_hap_desc.isConfigure) {
        return &gv_hap_desc.accessory;
    }
    return NULL;
}

const HAPAccessory *const *lhap_get_bridged_accessories(void)
{
    if (gv_hap_desc.isConfigure) {
        return (const HAPAccessory *const *)gv_hap_desc.bridgedAccessories;
    }
    return NULL;
}

size_t lhap_get_attribute_count(void)
{
    if (gv_hap_desc.isConfigure) {
        return gv_hap_desc.attributeCount;
    }
    return 0;
}
