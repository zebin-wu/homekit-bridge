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

#include <lualib.h>
#include <lauxlib.h>

#include "AppInt.h"
#include "lhaplib.h"
#include "DB.h"

#define HAP_ARRAY_LEN(x)        (sizeof(x) / sizeof(*(x)))
#define HAP_MALLOC(size)        malloc(size)
#define HAP_FREE(p)             do { if (p) { free((void *)p); (p) = NULL; } } while (0)

/**
 * Table key-value.
*/
struct table_kv {
    const char *key;    /* key */
    int type;   /* value type */
    /**
     * This function will be called when the key is parsed,
     * and the value is at the top of the stack.
     *
     * @param L 'per thread' state
     * @param kv the point to current key-value
     * @param arg the extra argument
    */
    bool (*cb)(lua_State *L, struct table_kv *kv, void *arg);
};

/**
 * Function type.
*/
enum func_type {
    FUNC_TYPE_IDENTIFY,
};

/**
 * LUA ref function.
*/
struct ref_func {
    int id; /* ref id */
    enum func_type type;
    void *parent; 
    struct ref_func *next;
};

/**
 * LUA light userdata.
*/
struct lhap_userdata {
    const char *name;
    void *userdata;
};

static const char *lua_type_string[] = {
    [LUA_TNIL] = "nil",
    [LUA_TBOOLEAN] = "boolean",
    [LUA_TLIGHTUSERDATA] = "light userdata",
    [LUA_TNUMBER] = "number",
    [LUA_TSTRING] = "string",
    [LUA_TTABLE] = "table",
    [LUA_TFUNCTION] = "function",
    [LUA_TUSERDATA] = "userdata",
    [LUA_TTHREAD] = "thread"
};

static struct lhap_userdata userdataServices[] = {
    {"AccessoryInformationService", (void *)&accessoryInformationService},
    {"HapProtocolInformationService", (void *)&hapProtocolInformationService},
    {"PairingService", (void *)&pairingService},
};

static struct hap_desc {
    bool isstarted:1;
    HAPAccessoryServerRef *server;
    HAPAccessory accessory;
    HAPAccessory **bridgedAccessories;
    struct ref_func *head;
} gv_hap_desc;

static bool
lhap_check_is_valid_userdata(void *userdata, struct lhap_userdata tab[], int len)
{
    for (int i = 0; i < len; i++) {
        if (userdata == (tab + i)->userdata) {
            return true;
        }
    }
    return false;
}

static bool lhap_check_is_valid_service(HAPService *service)
{
    return lhap_check_is_valid_userdata(service,
        userdataServices, HAP_ARRAY_LEN(userdataServices));
}

static bool
lhap_register_ref_func(lua_State *L, int index, void *parent, enum func_type type)
{
    struct ref_func **t = &(gv_hap_desc.head);
    struct ref_func *new;

    lua_pushvalue(L, index);
    int ref_id = luaL_ref(L, LUA_REGISTRYINDEX);
    if (ref_id == LUA_REFNIL) {
        return false;
    }

    // find the function
    while (*t != NULL) {
        if ((*t)->id == ref_id) {
            return false;
        }
        t = &(*t)->next;
    }
    new = HAP_MALLOC(sizeof(*new));
    if (!new) {
        return false;
    }
    new->id = ref_id;
    new->type = type;
    new->parent = parent;
    new->next = NULL;
    *t = new;
    return true;
}

static void lhap_del_func_list(lua_State *L, struct ref_func **head)
{
    struct ref_func *t;
    while (*head) {
        t = *head;
        luaL_unref(L, LUA_REGISTRYINDEX, t->id);
        free(t);
        head = &(*head)->next;
    }
    *head = NULL;
}

static bool lhap_push_ref_func(lua_State *L, void *parent, enum func_type type)
{
    struct ref_func *t = gv_hap_desc.head;
    while (t != NULL) {
        if (t->parent == parent && t->type == type) {
            break;
        }
        t = t->next;
    }
    if (!t) {
        return false;
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, t->id);
    return true;
}

/* return a new string copy from str */
static char *lhap_new_str(const char *str)
{
    if (!str) {
        return NULL;
    }
    char *copy = HAP_MALLOC(strlen(str) + 1);
    return copy ? strcpy(copy, str) : NULL;
}

static struct table_kv *
lhap_lookup_kv_by_name(struct table_kv *kv_tab, const char *name)
{
    for (;kv_tab->key != NULL; kv_tab++) {
        if (!strcmp(kv_tab->key, name)) {
            return kv_tab;
        }
    }
    return NULL;
}

static bool
lhap_traverse_table(lua_State *L, int index, struct table_kv *kvs, void *arg)
{
    // Push another reference to the table on top of the stack (so we know
    // where it is, and this function can work for negative, positive and
    // pseudo indices
    lua_pushvalue(L, index);
    // stack now contains: -1 => table
    lua_pushnil(L);
    // stack now contains: -1 => nil; -2 => table
    while (lua_next(L, -2)) {
        // stack now contains: -1 => value; -2 => key; -3 => table
        // copy the key so that lua_tostring does not modify the original
        lua_pushvalue(L, -2);
        // stack now contains: -1 => key; -2 => value; -3 => key; -4 => table
        struct table_kv *kv = lhap_lookup_kv_by_name(kvs, lua_tostring(L, -1));
        HAPLogDebug(&kHAPLog_Default,
            "%s: Match the key \"%s\".", __func__, kv->key);
        // pop copy of key
        lua_pop(L, 1);
        // stack now contains: -1 => value; -2 => key; -3 => table
        if (kv) {
            if (lua_type(L, -1) != kv->type) {
                HAPLogError(&kHAPLog_Default,
                    "%s: invalid type, %s: %s", __func__,
                    kv->key, lua_type_string[kv->type]);
                lua_pop(L, 2);
                return false;
            }
            if (kv->cb) {
                if (kv->cb(L, kv, arg) == false) {
                    HAPLogError(&kHAPLog_Default,
                        "%s: Failed to process the key \"%s\"",
                        __func__, kv->key);
                    lua_pop(L, 2);
                    return false;
                } else {
                    HAPLogDebug(&kHAPLog_Default,
                        "%s: Success to process the key \"%s\".",
                        __func__, kv->key);
                }
            }
        }
        // pop value, leaving original key
        lua_pop(L, 1);
        // stack now contains: -1 => key; -2 => table
    }
    // stack now contains: -1 => table (when lua_next returns 0 it pops the key
    // but does not push anything.)
    // Pop table
    lua_pop(L, 1);
    // Stack is now the same as it was on entry to this function
    return true;
}

static bool
lhap_traverse_array(lua_State *L, int index, bool (*arr_cb)(lua_State *L, int i, void *arg), void *arg)
{
    if (!arr_cb) {
        return false;
    }

    lua_pushvalue(L, index);
    lua_pushnil(L);
    for (int i = 0; lua_next(L, -2); lua_pop(L, 1), i++) {
        if (!arr_cb(L, i, arg)) {
            lua_pop(L, 2);
            return false;
        }
    }
    lua_pop(L, 1);
    return true;
}

static bool
accessory_aid_cb(lua_State *L, struct table_kv *kv, void *arg)
{
    ((HAPAccessory *)arg)->aid = lua_tonumber(L, -1);
    return true;
}

static bool
accessory_category_cb(lua_State *L, struct table_kv *kv, void *arg)
{
    ((HAPAccessory *)arg)->category = lua_tonumber(L, -1);
    return true;
}

static bool
accessory_name_cb(lua_State *L, struct table_kv *kv, void *arg)
{
    char **name = (char **)&(((HAPAccessory *)arg)->name);
    return (*name = lhap_new_str(lua_tostring(L, -1))) ?
        true : false;
}

static bool
accessory_manufacturer_cb(lua_State *L, struct table_kv *kv, void *arg)
{
    char **manufacturer = (char **)&(((HAPAccessory *)arg)->manufacturer);
    return (*manufacturer = lhap_new_str(lua_tostring(L, -1))) ?
        true : false;
}

static bool
accessory_model_cb(lua_State *L, struct table_kv *kv, void *arg)
{
    char **model = (char **)&(((HAPAccessory *)arg)->model);
    return (*model = lhap_new_str(lua_tostring(L, -1))) ?
        true : false;
}

static bool
accessory_serialnumber_cb(lua_State *L, struct table_kv *kv, void *arg)
{
    char **serialNumber = (char **)&(((HAPAccessory *)arg)->serialNumber);
    return (*serialNumber = lhap_new_str(lua_tostring(L, -1))) ?
        true : false;
}

static bool
accessory_firmwareversion_cb(lua_State *L, struct table_kv *kv, void *arg)
{
    char **firmwareVersion = (char **)&(((HAPAccessory *)arg)->firmwareVersion);
    return (*firmwareVersion = lhap_new_str(lua_tostring(L, -1))) ?
        true : false;
}

static bool
accessory_hardwareversion_cb(lua_State *L, struct table_kv *kv, void *arg)
{
    char **hardwareVersion = (char **)&(((HAPAccessory *)arg)->hardwareVersion);
    return (*hardwareVersion = lhap_new_str(lua_tostring(L, -1))) ?
        true : false;
}

static struct table_kv service_kvs[] = {
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
    HAPService *s = HAP_MALLOC(sizeof(HAPService));
    if (!s) {
        return false;
    }
    memset(s, 0, sizeof(HAPService));
    services[i] = s;
    if (!lhap_traverse_table(L, -1, service_kvs, s)) {
        return false;
    }
    return true;
}

static bool
accessory_services_cb(lua_State *L, struct table_kv *kv, void *arg)
{
    HAPAccessory *accessory = arg;
    HAPService ***pservices = (HAPService ***)&(accessory->services);

    // Get the array length.
    size_t len = lua_rawlen(L, -1);
    if (len == 0) {
        *pservices = NULL;
        return true;
    }

    HAPService **services = HAP_MALLOC(sizeof(HAPService *) * (len + 1));
    if (!services) {
        goto err;
    }
    memset(services, 0, sizeof(HAPService *) * (len + 1));

    if (!lhap_traverse_array(L, -1, accessory_services_arr_cb, services)) {
        goto err1;
    }

    *pservices = services;
    return true;
err1:
    for (HAPService **s = services; *s != NULL; s++) {
        if (!lhap_check_is_valid_service(*s)) {
            reset_service(*s);
            HAP_FREE(*s);
        }
    }
    HAP_FREE(services);
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

    if (!lhap_push_ref_func(L,
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
accessory_cbs_identify_cb(lua_State *L, struct table_kv *kv, void *arg)
{
    if (!lhap_register_ref_func(L, -1,
        &(((HAPAccessory *)arg)->callbacks.identify),
        FUNC_TYPE_IDENTIFY)) {
        return false;
    }
    ((HAPAccessory *)arg)->callbacks.identify = accessory_identify_cb;
    return true;
}

static struct table_kv accessory_callbacks_kvs[] = {
    {"identify", LUA_TFUNCTION, accessory_cbs_identify_cb},
    {NULL, -1, NULL},
};

static bool
accessory_callbacks_cb(lua_State *L, struct table_kv *kv, void *arg)
{
    return lhap_traverse_table(L, -1, accessory_callbacks_kvs, arg);
}

static struct table_kv accessory_kvs[] = {
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
    HAP_FREE(accessory->name);
    HAP_FREE(accessory->manufacturer);
    HAP_FREE(accessory->model);
    HAP_FREE(accessory->serialNumber);
    HAP_FREE(accessory->firmwareVersion);
    HAP_FREE(accessory->hardwareVersion);
}

bool accessories_arr_cb(lua_State *L, int i, void *arg)
{
    HAPAccessory **accessories = arg;
    if (!lua_istable(L, -1)) {
        return false;
    }
    HAPAccessory *a = HAP_MALLOC(sizeof(HAPAccessory));
    if (!a) {
        return false;
    }
    memset(a, 0, sizeof(HAPAccessory));
    accessories[i] = a;
    if (!lhap_traverse_table(L, -1, accessory_kvs, a)) {
        return false;
    }
    return true;
}

/**
 * start(accessory: table, bridgedAccessories: table,
 * configurationChanged: boolean) -> boolean
 *
 * If the category of the accessory is bridge, the parameters
 * bridgedAccessories and configurationChanged are valid.
*/
static int hap_start(lua_State *L)
{
    size_t len;
    struct hap_desc *desc = &gv_hap_desc;
    HAPAccessory *accessory = &desc->accessory;

    if (!desc->server) {
        goto err;
    }

    if (desc->isstarted) {
        goto err;
    }

    if (!lua_istable(L, 1)) {
        goto err;
    }

    if (!lhap_traverse_table(L, 1, accessory_kvs, accessory)) {
        HAPLogError(&kHAPLog_Default,
            "%s: Failed to generate accessory structure from table accessory.",
            __func__);
        goto err1;
    }

    if (accessory->category != kHAPAccessoryCategory_Bridges) {
        HAPAccessoryServerStart(desc->server, &desc->accessory);
        lua_pushboolean(L, true);
        return 1;
    }

    if (!lua_istable(L, 2) || !lua_isboolean(L, 3)) {
        goto err1;
    }

    len = lua_rawlen(L, 2);
    if (len == 0) {
        HAPAccessoryServerStart(desc->server, &desc->accessory);
        lua_pushboolean(L, true);
        return 1;
    }

    desc->bridgedAccessories =
        HAP_MALLOC(sizeof(HAPAccessory *) * (len + 1));
    if (!desc->bridgedAccessories) {
        goto err1;
    }
    memset(desc->bridgedAccessories, 0,
        sizeof(HAPAccessory *) * (len + 1));

    if (!lhap_traverse_array(L, 2, accessories_arr_cb,
        desc->bridgedAccessories)) {
        goto err2;
    }

    HAPAccessoryServerStartBridge(desc->server, &desc->accessory,
        (const HAPAccessory *const *)desc->bridgedAccessories,
        lua_toboolean(L, 3));
    lua_pushboolean(L, true);
    return 1;

err2:
    for (HAPAccessory **pa = desc->bridgedAccessories; *pa != NULL; pa++) {
        reset_accessory(*pa);
        HAP_FREE(*pa);
    }
    HAP_FREE(desc->bridgedAccessories);
err1:
    reset_accessory(accessory);
err:
    lua_pushboolean(L, false);
    return 1;
}

/**
 * stop() -> boolean
*/
static int hap_stop(lua_State *L)
{
    struct hap_desc *desc = &gv_hap_desc;

    if (!desc->server) {
        lua_pushboolean(L, false);
        return 1;
    }

    if (desc->isstarted) {
        lua_pushboolean(L, false);
        return 1;
    }

    HAPAccessoryServerStop(desc->server);

    /* clean all accessory */
    reset_accessory(&desc->accessory);
    for (HAPAccessory **pa = desc->bridgedAccessories;
        *pa != NULL; pa++) {
        reset_accessory(*pa);
        HAP_FREE(*pa);
    }
    HAP_FREE(desc->bridgedAccessories);

    /* clean all ref function */
    lhap_del_func_list(L, &desc->head);

    lua_pushboolean(L, true);
    return 1;
}

/* Create an enumeration table. */
static void create_enum_table(lua_State *L, const char *enum_array[], int len)
{
    lua_newtable(L);
    for (int i = 0; i < len; i++) {
        if (enum_array[i]) {
            lua_pushstring(L, enum_array[i]);
            lua_pushnumber(L, i);
            lua_settable(L, -3);
        }
    }
}

static const char *enumAccessoryCategory[] = {
    "BridgedAccessory",
    "Other",
    "Bridges",
    "Fans",
    "GarageDoorOpeners",
    "Lighting",
    "Locks",
};

static const char *enumError[] = {
    "None",
    "Unknown",
    "InvalidState",
    "InvalidData",
    "OutOfResources",
    "NotAuthorized",
    "Busy",
};

static const luaL_Reg haplib[] = {
    {"start", hap_start},
    {"stop", hap_stop},
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
    create_enum_table(L, enumAccessoryCategory,
        HAP_ARRAY_LEN(enumAccessoryCategory));
    lua_setfield(L, -2, "AccessoryCategory");

    /* set Error */
    create_enum_table(L, enumError,
        HAP_ARRAY_LEN(enumError));
    lua_setfield(L, -2, "Error");

    /* set services */
    for (int i = 0; i < HAP_ARRAY_LEN(userdataServices); i++) {
        lua_pushlightuserdata(L, (userdataServices + i)->userdata);
        lua_setfield(L, -2, (userdataServices + i)->name);
    }
    return 1;
}

void lhap_set_server(HAPAccessoryServerRef *server)
{
    if (!gv_hap_desc.server) {
        gv_hap_desc.server = server;
    }
}

const HAPAccessory *lhap_get_accessory(void)
{
    return &gv_hap_desc.accessory;
}
