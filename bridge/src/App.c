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
#include "lc.h"
#include "lhaplib.h"
#include "lpallib.h"
#include "lloglib.h"

// Suffix of lua script file name.
#define LUA_SCRIPTS_SUFFIX ".luac"

// Generate lua script path.
#define GEN_LUA_SCRIPT_PATH(name, dir, buf, len) \
    snprintf(buf, len, "%s/%s" LUA_SCRIPTS_SUFFIX, dir, name)

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

/**
 * Global accessory configuration.
 */
typedef struct {
    HAPAccessoryServerRef* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
} AccessoryConfiguration;

static ApplicationContext appContext;
static AccessoryConfiguration accessoryConfiguration;

static const luaL_Reg loadedlibs[] = {
    {LUA_GNAME, luaopen_base},
    {LUA_LOADLIBNAME, luaopen_package},
    {LUA_COLIBNAME, luaopen_coroutine},
    {LUA_TABLIBNAME, luaopen_table},
    {LUA_IOLIBNAME, luaopen_io},
    {LUA_OSLIBNAME, luaopen_os},
    {LUA_STRLIBNAME, luaopen_string},
    {LUA_MATHLIBNAME, luaopen_math},
    {LUA_UTF8LIBNAME, luaopen_utf8},
    {LUA_DBLIBNAME, luaopen_debug},
    {LUA_HAPNAME, luaopen_hap},
    {LUA_PALNAME, luaopen_pal},
    {LUA_LOGNAME, luaopen_log},
    {NULL, NULL}
};

size_t AppLuaEntry(const char *work_dir) {
    HAPPrecondition(work_dir);

    lhap_initialize();

    char path[256];
    // set work dir to env LUA_PATH
    GEN_LUA_SCRIPT_PATH("?", work_dir, path, sizeof(path));
    if (setenv("LUA_PATH", path, 1)) {
        HAPLogError(&kHAPLog_Default, "Failed to set env LUA_PATH.");
    }

    lua_State *L  = luaL_newstate();
    if (L == NULL) {
        HAPLogError(&kHAPLog_Default, "Cannot create state: not enough memory");
        return 0;
    }

    // load libraries
    for (const luaL_Reg *lib = loadedlibs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* remove lib */
    }

    // run main scripts
    GEN_LUA_SCRIPT_PATH("main", work_dir, path, sizeof(path));
    int status = luaL_dofile(L, path);
    lc_collectgarbage(L);
    if (status != LUA_OK) {
        const char *msg = lua_tostring(L, -1);
        lua_writestringerror("main.lua: %s\n", msg);
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

    appContext.L = L;
    return lhap_get_attribute_count();
err:
    HAPFatalError();
}

void AppLuaClose(void) {
    lua_State *L = appContext.L;
    if (L) {
        lc_remove_all_callbacks(L);
        lhap_deinitialize(L);
        lua_close(L);
        appContext.L = NULL;
    }
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

void AccessoryServerHandleUpdatedState(HAPAccessoryServerRef* server, void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(appContext.L);
    lhap_server_handle_update_state(appContext.L, HAPAccessoryServerGetState(server));
}

void AccessoryServerHandleSessionAccept(
        HAPAccessoryServerRef* server,
        HAPSessionRef* session,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(appContext.L);
    lhap_server_handle_session_accept(appContext.L);
}

void AccessoryServerHandleSessionInvalidate(
        HAPAccessoryServerRef* server,
        HAPSessionRef* session,
        void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(appContext.L);
    lhap_server_handle_session_invalidate(appContext.L);
}

void AppInitialize(
        HAPAccessoryServerOptions* hapAccessoryServerOptions,
        HAPPlatform* hapPlatform,
        HAPAccessoryServerCallbacks* hapAccessoryServerCallbacks,
        void* _Nonnull * _Nonnull pcontext) {
    HAPPrecondition(pcontext);
    *pcontext = &appContext;

    // Display setup code.
    HAPSetupCode setupCode;
    HAPPlatformAccessorySetupLoadSetupCode(hapPlatform->accessorySetup, &setupCode);
    HAPLogInfo(&kHAPLog_Default, "Setup code: %s", setupCode.stringValue);
}

void AppDeinitialize() {
    /*no-op*/
}
