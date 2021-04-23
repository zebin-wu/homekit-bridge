// Copyright (c) 2021 KNpTrue and homekit-bridge contributors
//
// Licensed under the MIT License.
// You may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of homekit-bridge project authors.

#include <stdlib.h>

#include <HAP.h>
#include <HAPAccessorySetup.h>
#include <lauxlib.h>
#include <lualib.h>

#include <App.h>

#include "AppInt.h"
#include "lapi.h"
#include "lhaplib.h"
#include "lpallib.h"
#include "lloglib.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Domain used in the key value store for application data.
 *
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreDomain_Configuration ((HAPPlatformKeyValueStoreDomain) 0x00)

/**
 * Key used in the key value store to store the configuration state.
 *
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreKey_Configuration_State ((HAPPlatformKeyValueStoreDomain) 0x00)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Global accessory configuration.
 */
typedef struct {
    HAPAccessoryServerRef* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
} AccessoryConfiguration;

static AccessoryContext context;
static AccessoryConfiguration accessoryConfiguration;

static const luaL_Reg loadedlibs[] = {
    {LUA_HAPNAME, luaopen_hap},
    {LUA_PALNAME, luaopen_pal},
    {LUA_LOGNAME, luaopen_log},
    {NULL, NULL}
};

//----------------------------------------------------------------------------------------------------------------------

size_t AppLuaEntry(const char *work_dir)
{
    HAPPrecondition(work_dir);

    char path[256];
    // set work dir to env LUA_PATH
    snprintf(path, sizeof(path), "%s/?.lua", work_dir);
    if (setenv("LUA_PATH", path, 1)) {
        HAPLogError(&kHAPLog_Default, "Failed to set env LUA_PATH.");
    }

    lua_State *L  = luaL_newstate();
    if (L == NULL) {
        HAPLogError(&kHAPLog_Default, "Cannot create state: not enough memory");
        return 0;
    }

    // load libraries
    luaL_openlibs(L);
    for (const luaL_Reg *lib = loadedlibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
    }

    // run main.lua
    snprintf(path, sizeof(path), "%s/main.lua", work_dir);
    int status = luaL_dofile(L, path);
    lapi_collectgarbage(L);
    if (status != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        lua_writestringerror("%s\n", msg);
        lua_pop(L, 1);
        goto err;
    }

    if (!lua_isboolean(L, -1)) {
        HAPLogError(&kHAPLog_Default, "main.lua returned is not a boolean.");
        goto err;
    }

    if (!lua_toboolean(L, -1)) {
        HAPLogError(&kHAPLog_Default, "Failed to configure.");
        goto err;
    }

    context.L = L;
    return lhap_get_attribute_count();
err:
    lua_close(L);
    return 0;
}

void AppCreate(HAPAccessoryServerRef* server, HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;
}

void AppRelease(void) {
}

void AppAccessoryServerStart(void) {
    const HAPAccessory *accessory = lhap_get_accessory();
    const HAPAccessory *const *bridgedAccessories = lhap_get_bridged_accessories();

    if (bridgedAccessories) {
        HAPAccessoryServerStartBridge(accessoryConfiguration.server, accessory,
            bridgedAccessories, true);
    } else {
        HAPAccessoryServerStart(accessoryConfiguration.server, accessory);
    }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessoryServerHandleUpdatedState(HAPAccessoryServerRef* server, void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(context);

    switch (HAPAccessoryServerGetState(server)) {
        case kHAPAccessoryServerState_Idle: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Idle.");
            return;
        }
        case kHAPAccessoryServerState_Running: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Running.");
            return;
        }
        case kHAPAccessoryServerState_Stopping: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Stopping.");
            return;
        }
    }
    HAPFatalError();
}

void AppInitialize(
        HAPAccessoryServerOptions* hapAccessoryServerOptions,
        HAPPlatform* hapPlatform,
        HAPAccessoryServerCallbacks* hapAccessoryServerCallbacks,
        void* _Nonnull * _Nonnull pcontext) {
    HAPPrecondition(pcontext);
    *pcontext = &context;

    // Display setup code.
    HAPSetupCode setupCode;
    HAPPlatformAccessorySetupLoadSetupCode(hapPlatform->accessorySetup, &setupCode);
    HAPLogInfo(&kHAPLog_Default, "Setup code: %s", setupCode.stringValue);
}

void AppDeinitialize() {
    /*no-op*/
}
